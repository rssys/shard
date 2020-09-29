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

struct func_handler {
	int idx;
	two_args handler;
};

void init_fh(struct func_handler * fh, int idx, two_args handler) {
	fh->idx = idx;
	fh->handler = handler;
}

typedef void (*initializer) (struct func_handler * fh, int idx, two_args handler);

struct func_handler fh_array[4];

void initialize(/*initializer init_fh, */struct func_handler * fh) {
	init_fh(&fh[0], 0, handle_zero);
	init_fh(&fh[1], 1, (two_args) handle_one);
	init_fh(&fh[2], 2, (two_args) handle_two);
	init_fh(&fh[3], 3, handle_three);
}

int main(int argc, char ** argv) {
	initialize(/*&init_fh, */fh_array);
	int idx = atoi(argv[1]);
	fh_array[idx].handler(argv[2], argv[3]);
	return 0;
}