.PHONY: main test benchmark utx

main:
	gcc main.c -o main
	./main

test:
	gcc test.c -o test
	./test

benchmark:
	gcc benchmark.c -o benchmark -O2
	./benchmark

utx:
	gcc utx.c -o utx -O3
	./utx

clean:
	rm test main
