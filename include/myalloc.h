#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>


// Memory management functions //

// Returns a pointer to the body of a memory block
void* myMalloc(size_t size);
// Frees the block associated to the pointer
void myFree(void* ptr);
// Frees the block associated to the pointer and reallocate it for new data
void* myRealloc(void* ptr, size_t size);


// Debug functions //

// Shows the content of a block by displaying the asci representation of each of its bytes
void print_block_content(void* ptr);
// Shows which blocks of memory are used ( o for free and x if used)
void print_small_blocks_used();
// Prints the list of free large block on the heap with their size, address and header
void print_free_large_blocks();


// Memory reading and writing functions //

// Returns int at a pointer address in memory after checking that the pointer is valid
int read_safe_int_small(void* ptr);
// Returns char at a pointer address in memory after checking that the pointer is valid
char read_safe_char_small(void* ptr);
// Writes int at a pointer address in memory after checking that the pointer is valid
void write_safe_int_small(void* ptr,int value);
// Writes char at a pointer address in memory after checking that the pointer is valid
void write_safe_char_small(void* ptr,char value);


// Test functions //

void test_general();
void test_free1();
void test_free2();
void test_malloc1();
void test_malloc2();
void test_realloc();
void test_header();
void test_large_block1();
void test_large_block2();
void speed_test(size_t testNB);


