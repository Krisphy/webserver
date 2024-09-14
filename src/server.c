// My Webserver Pet Project
// Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <signal.h>
// Defines
#define BUFFER_SIZE 4096

void 
send_error_response(int fd, int status_code, const char* status_text) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), 
             "HTTP/1.1 %d %s\r\nContent-Type: text/plain\r\n\r\n%s",
             status_code, status_text, status_text);
    write(fd, response, strlen(response));
}

const char* 
file_type(const char* file_path) {
    const char* extension = strrchr(file_path, '.');
    if (extension != NULL) {
        if (strcmp(extension, ".html") == 0) return "text/html";
        if (strcmp(extension, ".css") == 0) return "text/css";
        if (strcmp(extension, ".js") == 0) return "application/javascript";
        if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0) return "image/jpeg";
        if (strcmp(extension, ".png") == 0) return "image/png";
        if (strcmp(extension, ".gif") == 0) return "image/gif";
    }
    return "application/octet-stream";  // Default content type
}

void 
send_file(int fd, const char* file_path) {
    int file_fd = open(file_path, O_RDONLY);
    if (file_fd < 0) {
        send_error_response(fd, 404, "Not Found");
        return;
    }
    // Get file size through stat
    struct stat file_stat;
    if (fstat(file_fd, &file_stat) < 0) {
        send_error_response(fd, 500, "Internal Error: Failed to get file size");
        close(file_fd);
        return;
    }
    long file_size = file_stat.st_size;
    // Send header
    char buffer[BUFFER_SIZE];
    const char* content_type = file_type(file_path);
    snprintf(buffer, sizeof(buffer),
             "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",
             content_type, file_size);
    write(fd, buffer, strlen(buffer));
    int bytes_read;
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer))) > 0) {
        write(fd, buffer, bytes_read);
    }
    // Close the file descriptor
    close(file_fd);
}

void* 
handle_client_request(void* arg) {
    int fd = *(int*)arg;
    char buffer[BUFFER_SIZE];
    // Reading the request from the socket
    int bytes_read = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        fprintf(stderr, "Error reading from socket or connection closed\n");
        close(fd);
        free(arg);
        return NULL;
    }
    // Adding null terminator to the end of the buffer just in case
    buffer[bytes_read] = '\0';
    printf("Received request:\n%s\n", buffer);
    regex_t regex;
    // Need size 3, one spot for the overall match, one spot for the HTTP method, and one spot for the path
    regmatch_t matches[3];
    // Compile regex expression
    // Two groups: 1st group is the HTTP method (Just GET for now), 2nd group is the path
    // Anchored to start, then get series of capital letters for header, then a space, then the path, then HTTP version
    if (regcomp(&regex, "^([A-Z]+) (/[^ ]*) HTTP/1\\.[01]", REG_EXTENDED) != 0) {
        fprintf(stderr, "Could not compile regex\n");
        send_error_response(fd, 500, "Internal Server Error");
        close(fd);
        free(arg);
        return NULL;
    }
    // Execute regex; get matches
    if (regexec(&regex, buffer, 3, matches, 0) == 0) {
        char method[8] = {0};
        char path[256] = {0};
        // Extract method and path
        strncpy(method, buffer + matches[1].rm_so, matches[1].rm_eo - matches[1].rm_so);
        strncpy(path, buffer + matches[2].rm_so, matches[2].rm_eo - matches[2].rm_so);
        if (strcmp(method, "GET") == 0) {
            char file_path[256] = "./host_website";
            if (strcmp(path, "/") == 0) {
                // Root path, serve index page
                strcat(file_path, "/index.html");
            } else {
                // Third argument is a long way to say "if the path is too long, don't overflow the buffer"
                strncat(file_path, path, sizeof(file_path) - strlen(file_path) - 1);
            }
            send_file(fd, file_path);
        } else {
            send_error_response(fd, 405, "Method Not Supported");
        }
    } else {
        // If regexec fails to find match
        send_error_response(fd, 400, "Bad HTTP Request");
    }
    regfree(&regex);
    close(fd);
    free(arg);
    return NULL;
}

// TO DO: This is meant for cleaning up resources, restructure as needed to free allocations and close sockets
void
signal_handler(int signal) {
    if (signal == SIGINT)
    {
        // $ Ctrl + C
        printf("\nDetected: SIGINT ... closing program normally\n");
        exit(0);
    }
    else if (signal == SIGTERM)
    {
        // $ Ctrl + \ 
        printf("\nDetected: SIGTERM ... closing program normally\n");
        exit(0);
    }
    else if (signal == SIGQUIT)
    {
        // $ kill <pid>
        printf("\nDetected: SIGQUIT ... closing program normally\n");
        exit(0);
    }
    else if (signal == SIGPIPE)
    { 
        // broken pipe
        fprintf(stderr, "\nDetected: SIGPIPE ... issue in writing to socket, closing program\n");
        exit(1);
    }
}

int 
main(int argc, char* args[]) {
    // User specified port; defaults 8080
    // TO DO: Add error handling here for risky ports / invalid args
    const uint16_t PORT = (argc == 2) ? atoi(args[1]) : 8080u;
    // Setup signal handling
    signal (SIGINT, signal_handler);    // $ Ctrl + C
    signal (SIGTERM, signal_handler);   // $ Ctrl + \ 
    signal (SIGQUIT, signal_handler);   // $ kill <pid>
    signal (SIGPIPE, signal_handler);   // broken pipe
    // Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    // Enable port reuse
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setting socket to reuse port failed");
        exit(EXIT_FAILURE);
    }
    // Bind socket to a localhost and user defined port
    struct sockaddr_in address = {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(PORT)};
    int addrlen = sizeof(address);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // Listen for incoming connections
    if (listen(server_fd, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    // Information prints: (port, process id)
    printf("Listening on port: %d\n", PORT);
    printf("Process id: %ld\n", (long)getpid());
    while (1) {
        // Accept and handle incoming connections
        int* user_socket = (int *)malloc(sizeof(int));
        *user_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (*user_socket == -1) {
            free(user_socket);
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        printf("Accepted connection\n\n");
        // Handle client request in a separate thread
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client_request, user_socket);
        pthread_detach(thread);
    }
}
