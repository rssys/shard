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

two_args fh_array[4];

void initialize_array() {
	fh_array[0] = (two_args) handle_zero;
	fh_array[1] = (two_args) handle_one;
	fh_array[2] = (two_args) handle_two;
	fh_array[3] = (two_args) handle_three;
}

int main(int argc, char ** argv) {
	int idx = atoi(argv[1]);
	initialize_array();
	fh_array[idx](argv[2], argv[3]);
	return 0;
}