CC      = gcc
CFLAGS  = -O3 -march=native -funroll-loops -fomit-frame-pointer -flto \
           -DOPENSSL_API_COMPAT=0x10100000L
LDFLAGS = -lcrypto
TARGET  = sha1
SRC     = sha1.c

.PHONY: all clean run bench

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# รัน N รอบ: make run N=10
run: $(TARGET)
	./$(TARGET) $(N)

# วัดเวลา 5,555,555,555 รอบ บน core เดียว
bench: $(TARGET)
	time taskset -c 0 ./$(TARGET) 5555555555

clean:
	rm -f $(TARGET)
