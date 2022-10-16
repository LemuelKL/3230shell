CC=gcc

ALL=3230shell_3035782231 3230shell_3035782231.o pipe.o

pipe.o: pipe.c
	$(CC) -c $<

3230shell_3035782231.o: 3230shell_3035782231.c
	$(CC) -c $<

all: 3230shell_3035782231.o pipe.o
	$(CC) $^ -o 3230shell_3035782231

clean:
	rm $(ALL)
