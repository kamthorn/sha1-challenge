CC = gcc
CFLAGS = -O3 -msha -msse4.1
TARGET = sha1
SRC = sha1.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
