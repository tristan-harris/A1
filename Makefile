all: a1

a1: a1.c
	$(CC) a1.c -o a1 -Wall -Wextra -pedantic -std=c99

clean:
	rm a1
