#include<stdio.h>
#include<string.h>
#include<stdlib.h>

typedef void (*one_arg) (char *);
typedef void (*two_args) (char *, char *);

void handle_zero(char * arg1, char * arg2) {
	printf("handling zero with args %s, %s\n", arg1, arg2);
}

void handle_one(char * arg1) {
	printf("handling one with arg %s\n", arg1);
}

void handle_two(char * arg1) {
	printf("handling two with arg %s\n", arg1);
}

void handle_three(char * arg1, char * arg2) {
	printf("handling three with args %s, %s\n", arg1, arg2);
}

two_args fh = &handle_zero;

void initialize_with_one() {
	fh = (two_args) &handle_one;
}

void initialize_with_two() {
	fh = (two_args) &handle_two;
}

void initialize_with_three() {
	fh = &handle_three;
}

int main(int argc, char ** argv) {
	int idx = atoi(argv[1]);
	switch(idx) {
		case 1: initialize_with_one(); break;
		case 2: initialize_with_two(); break;
		case 3: initialize_with_three(); break;
	}
	fh(argv[2], argv[3]);
	return 0;
}