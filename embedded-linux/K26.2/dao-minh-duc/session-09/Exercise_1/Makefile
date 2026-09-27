CC = gcc
CFLAGS = -Wall -Wextra -g

TARGETS = sensor-daemon sensor-cli

all: $(TARGETS)

sensor-daemon: daemon.o
	$(CC) $(CFLAGS) -o $@ $^

sensor-cli: cli.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c sensor_shm.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
