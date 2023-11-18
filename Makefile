CC = gcc
CFLAGS = -Wall -Wextra -pedantic
SRC = main.c list.c
OBJ = $(SRC:.c=.o)
EXECUTABLE = app_test.o

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CC) $(CFLAGS) -pthread -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf main main.o app_test.o *~
