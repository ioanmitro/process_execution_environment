all: hw3 test

hw3: hw3.c
	gcc -Wall -g hw3.c -o hw3
test: test.c
	gcc -Wall -g test.c -o test
clean: 
	rm hw3 test
