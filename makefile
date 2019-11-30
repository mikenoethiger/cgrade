all: compile test 

compile: cgrade_test.c
	mkdir -p out
	gcc -o ./out/cgrade_test cgrade_test.c
	gcc -o ./out/cgrade cgrade_main.c

test: 
	./out/cgrade_test

clean:
	rm -rf out
