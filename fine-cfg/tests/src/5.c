#include<stdio.h>
#include<string.h>
#include<stdlib.h>

typedef void (*one_arg) (char *);
typedef void (*two_args) (char *, char *);


void handle_name(char * arg1, char * arg2) {
	printf("handling name with args %s, %s\n", arg1, arg2);
}

void handle_place(char * arg1) {
	printf("handling place with arg %s\n", arg1);
}

void handle_animal(char * arg1) {
	printf("handling animal with arg %s\n", arg1);
}

void handle_thing(char * arg1, char * arg2) {
	printf("handling thing with args %s, %s\n", arg1, arg2);
}

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

void (*fh_array0[]) (char * arg1, char * arg2) =  {(two_args) &handle_name, 
												  (two_args) &handle_place, 
								   				  (two_args) &handle_animal, 
								   				  (two_args) &handle_thing};

void (*fh_array1[]) (char * arg1, char * arg2) =  {(two_args) &handle_zero, 
												  (two_args) &handle_one, 
								   				  (two_args) &handle_two, 
								   				  (two_args) &handle_three};

int main(int argc, char ** argv) {
	int idx1 = atoi(argv[1]);
	int idx2 = atoi(argv[2]);
	two_args func = idx1 ? fh_array1[idx2] : fh_array0[idx2];
	func(argv[3], argv[4]);
	return 0;
}