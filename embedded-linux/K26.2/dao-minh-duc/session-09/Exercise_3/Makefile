CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lrt -lpthread

TARGETS = controller device

all: $(TARGETS)

controller: controller.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

device: device.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o $(TARGETS)

.PHONY: all clean
