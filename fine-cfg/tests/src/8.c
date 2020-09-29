#include<stdio.h>
#include<string.h>
#include<stdlib.h>

typedef void (*one_arg) (char *);
typedef void (*two_args) (char *, char *);

void handle_zero(char * arg1, char * arg2) {
	printf("handling zero with args %s, %s\n", arg1, arg2);
}

struct func_handler_1 {
  char * name;
  void (*handler) (char * arg1, char * arg2);
};

struct func_handler_2 {
	char * name;
	void (*handler) (char * arg1, char * arg2);
};

struct func_handler_1 fh = {"zero", (two_args) handle_zero};

struct func_handler_2 * fh_ptr = (struct func_handler_2 *) &fh;

int main(int argc, char ** argv) {
	fh_ptr->handler(argv[1], argv[2]);
	return 0;
}


struct Type_1 {
  /* field */
  void (*handler_1) (char * arg1, char * arg2);
	/* field */
};

struct Type_2 {
	/* field */
	void (*handler_2) (char * arg1, char * arg2);
	/* field */
};

struct Type_1 st1;

void foo() {
	struct Type_2 st2 = (struct Type_2) st1;
	st2.handler_2 = &baz;
}

void baz() {
	st1.handler_1();
}











