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

struct func_handler_1 {
  char * name;
  void (*handler) (char * arg1, char * arg2);
};

struct func_handler_2 {
  char * name;
  void (*handler) (char * arg1, char * arg2);
};

struct func_handler_2 * initialize_array() {
	struct func_handler_2 * fh_array = malloc(sizeof(struct func_handler_2) * 4);
	fh_array[0].name = malloc(100);
	memcpy(fh_array[0].name, "zero\0", 100);
	fh_array[0].handler = &handle_zero;
	fh_array[1].name = malloc(100);
	memcpy(fh_array[1].name, "one\0", 100);
	fh_array[1].handler = (two_args) &handle_one;
	fh_array[2].name = malloc(100);
	memcpy(fh_array[2].name, "two\0", 100);
	fh_array[2].handler = (two_args) &handle_two;
	fh_array[3].name = malloc(100);
	memcpy(fh_array[3].name, "three\0", 100);
	fh_array[3].handler = &handle_three;		
	return fh_array;
}

int main(int argc, char ** argv) {
	int idx = atoi(argv[1]);
	struct func_handler_1 * fh_array = (struct func_handler_1 *) initialize_array();
	fh_array[idx].handler(argv[2], argv[3]);
	return 0;
}




			
				// ASSUMING NUM_ADDRESS_TAKEN_FUNCS = 5
				// and foo having index 2 and being the only 
				// valid target for given call site
				
				switch_table_for_callsite = {
					&error,
					&error,
					&foo,
					&error,
					&error
				};