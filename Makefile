CC = gcc
CFLAGS = -Wall -Ilib/curl/include -Ilib/cjson/include
LDFLAGS = -lwininet

SRCS = src/cf_summary.c src/cjson.c lib/curl/src/curl.c
TARGET = cf_summary.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	del $(TARGET) 2>/dev/null || true

.PHONY: all clean