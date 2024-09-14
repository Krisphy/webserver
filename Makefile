# https://makefiletutorial.com/#makefile-cookbook

TARGET_EXEC := webserver
BUILD_DIR := ./build
SRC_DIRS := ./src
SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -r $(BUILD_DIR)
	rm -r $(TARGET_EXEC)

