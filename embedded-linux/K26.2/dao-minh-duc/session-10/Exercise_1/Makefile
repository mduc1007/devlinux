CC = gcc
CFLAGS = -Wall -Wextra -g

TARGETS = iot_server iot_client

all: $(TARGETS)

iot_server: iot_server.o
	$(CC) $(CFLAGS) -o $@ $^

iot_client: iot_client.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c iot_server.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
