CC = gcc
CFLAGS = -Wall -Wextra -g

TARGETS = config-writer config-reader

all: $(TARGETS)

config-writer: writer.o
	$(CC) $(CFLAGS) -o $@ $^

config-reader: reader.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c device_cfg.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
