CFLAGS=-g -Wall

run: bakergc
	./bakergc

bakergc: bakergc.o
