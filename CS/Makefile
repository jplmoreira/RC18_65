CC = gcc
CFLAGS = -Wall
EXECS = cs.exe

all: $(EXECS)

cs.exe: cs.o
	$(CC) $(CFLAGS) -o cs.exe cs.o utilities.o

cs.o: cs.c utilities.o
	$(CC) $(CFLAGS) -c cs.c

utilities.o: ../utilities.c ../utilities.h
	$(CC) $(CFLAGS) -c ../utilities.c

clean:
	rm *.o $(EXECS)
