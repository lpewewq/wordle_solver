CC = gcc
CFLAGS = -O3 # -fdiagnostics-color=always -g

main: main.o solver.o
	$(CC) solver.o main.o -lm -o main

clean:
	rm -f main
	rm -f *.o
