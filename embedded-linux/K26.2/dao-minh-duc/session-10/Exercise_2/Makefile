CC = gcc
CFLAGS = -Wall -Wextra -g

TARGETS = event_monitor event_client

all: $(TARGETS)

event_monitor: event_monitor.o
	$(CC) $(CFLAGS) -o $@ $^

event_client: event_client.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c event_monitor.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
