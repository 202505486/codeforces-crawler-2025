CC = gcc
CFLAGS = -Wall -Iku/curl/include -Iku/cjson/include
LDFLAGS = -lwininet

SRCS = code/cf_summary.c code/cJSON.c ku/curl/src/curl.c
TARGET = cf_summary.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	del $(TARGET) 2>nul || true

.PHONY: all clean
