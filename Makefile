CC=gcc

main.o: main.c
	$(CC) -c $<

3230shell: main.o
	$(CC) $^ -o $@ -lreadline

clean:
	rm $(EXE)
