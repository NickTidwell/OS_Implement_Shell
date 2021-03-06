CC = gcc
CFLAGS = -Wall -std=c99
#List of dependencies (.h)
DEPS =
#List of object files (.o)
OBJ = parser.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
make: $(OBJ)
	$(CC) -o shell $^ $(CFLAGS)

clean:
	rm *.o
	rm shell
