CC=gcc

main.o: main.c
	$(CC) -c $<

3230shell: main.o
	$(CC) $^ -o $@

clean:
	rm $(EXE)
