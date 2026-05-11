CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lsqlite3

TARGET = main

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LDFLAGS)

clean:
	rm -f $(TARGET)
