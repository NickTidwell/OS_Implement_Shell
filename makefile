CC = gcc
CFLAGS = -Wall -g
#List of dependencies (.h)
DEPS =
#List of object files (.o)
OBJ = parser.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
make: $(OBJ)
	$(CC) -o shell $^ $(CFLAGS)

clean:
	rm *.o;
	rm shell;