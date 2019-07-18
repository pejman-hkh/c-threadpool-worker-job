all: main

main:
	gcc main.c tpool.c -pthread

clean:
	rm -rf a.out