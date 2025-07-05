CC      = gcc

SRC_DIR = src
OBJ_DIR = build

CFLAGS = -Wall -Wextra
CFLAGS += $(shell pkg-config --cflags x11 xft)

LDFLAGS = $(shell pkg-config --libs x11 xft)

SRCS = $(shell find $(SRC_DIR) -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = controlcenter

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

install:
	install -m 755 $(TARGET) /usr/local/bin/

uninstall:
	rm /usr/local/bin/$(TARGET)

