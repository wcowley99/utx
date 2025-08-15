.PHONY: main test

main:
	gcc main.c -o main
	./main

test:
	gcc test.c -o test
	./test

clean:
	rm test main
