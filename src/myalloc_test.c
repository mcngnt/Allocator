#include "myalloc.h"

int main()
{

	printf("\n-------------------\n Small blocks general test : \n-------------------\n\n");

	test_general();

	printf("\n-------------------\n Small blocks free test 1 : \n-------------------\n\n");

	test_free1();

	printf("\n-------------------\n Small blocks free test 2 : \n-------------------\n\n");

	test_free2();

	printf("\n-------------------\n Small blocks malloc test : \n-------------------\n\n");

	test_malloc1();

	printf("\n-------------------\n Small blocks  realloc test : \n-------------------\n\n");

	test_realloc();

	printf("\n-------------------\n Small blocks header test : \n-------------------\n\n");

	test_header();

	printf("\n-------------------\n Large blocks test 1: \n-------------------\n\n");

	test_large_block1();

	printf("\n-------------------\n Large blocks test 2 : \n-------------------\n\n");

	test_large_block2();

	printf("\n-------------------\n Speed test : \n-------------------\n\n");

	speed_test(100000); // one hundred million tests

	// printf("poop");

	// while(1){};

	return 0;
}