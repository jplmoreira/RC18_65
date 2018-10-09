CC = gcc
CFLAGS = -Wall
EXECS = user.exe

all: $(EXECS)

user.exe: user.o
	$(CC) $(CFLAGS) -o user.exe user.o utilities.o

user.o: user.c utilities.o
	$(CC) $(CFLAGS) -c user.c

utilities.o: ../utilities.c ../utilities.h
	$(CC) $(CFLAGS) -c ../utilities.c

clean:
	rm *.o $(EXECS)
