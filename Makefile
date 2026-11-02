CC      = gcc
CFLAGS  = -O2 -Wall -pthread

.PHONY: all clean

all: pi_single pi_local pi_mutex pi_big

pi_single: src/pi_single.c src/common.h
	$(CC) $(CFLAGS) -o $@ src/pi_single.c

pi_local: src/pi_local.c src/common.h
	$(CC) $(CFLAGS) -o $@ src/pi_local.c

pi_mutex: src/pi_mutex.c src/common.h
	$(CC) $(CFLAGS) -o $@ src/pi_mutex.c

pi_big: src/pi_big.c
	$(CC) $(CFLAGS) -o $@ src/pi_big.c -lm

clean:
	rm -f pi_single pi_local pi_mutex pi_big
