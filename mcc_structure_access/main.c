#include <stdlib.h>
#include <stdio.h>

#include "asm.h"

struct test {
	unsigned int main;
	unsigned int arr[10];
};

static struct test t = {0};
static const char fmt[] = "value=%d\n";

int main(int argc, char *argv[]) {
	struct test *ptr_t = &t;
	printf("t.main=%d\n", ptr_t->main);
	ptr_t->main = 1;
	ptr_t->arr[0] = 0;
	ptr_t->arr[1] = 1;
	ptr_t->arr[2] = 2;
	ptr_t->arr[3] = 3;
	ptr_t->arr[4] = 4;
	ptr_t->arr[5] = 5;
	ptr_t->arr[6] = 6;
	ptr_t->arr[7] = 7;
	ptr_t->arr[8] = 8;
	ptr_t->arr[9] = 9;

	printf("t.main=%d\n", ptr_t->main);
	{
		int i;
		for (i = 0; i < 10; i++) {
			printf("arr[%d]=%d\n", i, ptr_t->arr[i]);
		}
	}
	/* assembly test */
	{
		unsigned int ret = 0;
		printf("before t.main=%#X\n", ptr_t->main);
		ret = set_bit_test(&t);
		printf("return value=%#X\n", ret);
		printf("after  t.main=%#X\n", ptr_t->main);
	}
	return 0;
}
