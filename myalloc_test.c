#include "myalloc.h"

int main()
{

	printf("\n-------------------\n General test : \n-------------------\n\n");

	test_general();

	printf("\n-------------------\n Free test 1 : \n-------------------\n\n");

	test_free1();

	printf("\n-------------------\n Free test 2 : \n-------------------\n\n");

	test_free2();

	printf("\n-------------------\n Malloc test 1 : \n-------------------\n\n");

	test_malloc1();

	printf("\n-------------------\n Malloc test 1 : \n-------------------\n\n");

	test_malloc1();

	printf("\n-------------------\n Malloc test 2 : \n-------------------\n\n");

	test_malloc2();

	printf("\n-------------------\n Realloc test : \n-------------------\n\n");

	test_realloc();

	printf("\n-------------------\n Header test : \n-------------------\n\n");

	header_test();

	return 0;
}