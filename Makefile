CC = gcc
CFLAGS = -Wall
DEBUG = -fdiagnostics-color=always -g
RELEASE = -O3
LDFLAGS = -lm
SOURCES = main.c solver.c solver_utility.c wordle.c
OBJECTS = $(SOURCES:.c=.o)
DEBUG_OBJECTS = $(addprefix debug_, $(OBJECTS))

all: debug release

release: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

debug: $(DEBUG_OBJECTS)
	$(CC) $(DEBUG_OBJECTS) $(LDFLAGS) -o $@

debug_%.o: %.c
	$(CC) $(CFLAGS) $(DEBUG) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(RELEASE) -c $< -o $@

clean:
	rm -f release debug *.o

.PHONY: clean all
