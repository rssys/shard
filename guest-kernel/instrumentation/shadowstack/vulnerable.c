/* vul.c by _6mO_HaCk */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void target() {
	printf("control flow hijacked\n");
}

				int vulnerable(char * in) {
					char buffer[10];
					strcpy(buffer, in);
					return 0;
				}

int main(int argc, char * argv[]) {
	if(argc < 2) {
		printf("Usage : %s buffer\n", argv[0]);
		exit(0);
	}
	vulnerable(argv[1]);
}
