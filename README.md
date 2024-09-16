# (linux) webserver: a from-scratch pet project

A pet project to get introduced to network sockets, threads, regex, and general C programming in the context of Unix-like systems

To build just run 'make' in the top level directory, a 'webserver' executable will be generated


Functionality to add:
- Better handling of clean up when system signals are received
- Epoll with a thread pool to avoid having to assign a new thread to each file descriptor
- Support for more HTTP header requests
- Secure encryption through OpenSSL library
