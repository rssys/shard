#include<stdio.h>
#include<string.h>
#include<stdlib.h>

typedef void (*two_args) (char *, char *);

void handle_zero(char * arg1, char * arg2) {
	printf("handling zero with args %s, %s\n", arg1, arg2);
}
void handle_one(char * arg1, char * arg2) {
	printf("handling one with args %s, %s\n", arg1, arg2);
}

struct func_handler {
  char * name;
  void (*handler0) (char * arg1, char * arg2);
  void (*handler1) (char * arg1, char * arg2);
};

struct top_level {
	int val;
	struct func_handler * fh;
};

char * names[] = {"zero", "first", "second", "third", "fourth"};
struct top_level tl_array[5];

void initialize() {
	for(int i = 0; i < 5; i++) {
		tl_array[i].val = i;
		tl_array[i].fh = malloc(sizeof(struct func_handler));
		tl_array[i].fh->name = names[i];
		tl_array[i].fh->handler0 = handle_zero;
		tl_array[i].fh->handler1 = handle_one;
	}
}

int main(int argc, char ** argv) {
	initialize();
	int idx = atoi(argv[1]);
	tl_array[idx].fh->handler0(tl_array[idx].fh->name, argv[2]);
	tl_array[idx].fh->handler1(tl_array[idx].fh->name, argv[2]);
	return 0;
}