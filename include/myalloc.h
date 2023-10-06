#include <stdio.h>


#define MAX_SMALL 100


#define SIZE_BLK_SMALL 128 - sizeof(size_t)

// Struct used to represent a block of memory
struct BlockSmall
{
	// The LSB of the header tells if the block is in use (LSB = 1) or not (LSB = 0)
	size_t header;
	// Body of the block
	char body[SIZE_BLK_SMALL];
};




// Memory test functions //

// Returns 1 if the pointer is within the boundaries of memory and else 0
int is_memory_safe(void* ptr);


// Memory management functions //

void initialize_memory();
// Returns a pointer to the body of a memory block
void* myMalloc(size_t size);
// Frees the block associated to the pointer
void myFree(void* ptr);
// Frees the block associated to the pointer and reallocate it for new data
void* myRealloc(void* ptr, size_t size);


// Memory reading and writing functions //

// Returns int at a pointer adress in memory after checking that the pointer is valid
int read_safe_int(void* ptr);
// Returns char at a pointer adress in memory after checking that the pointer is valid
char read_safe_char(void* ptr);
// Writes int at a pointer adress in memory after checking that the pointer is valid
void write_safe_int(void* ptr,int value);
// Writes char at a pointer adress in memory after checking that the pointer is valid
void write_safe_char(void* ptr,char value);


// Debug functions //

// Shows the content of a block by displaying the asci representation of each of its bytes
void print_block_content(unsigned int blockID);
// Shows which blocks of memory are used ( o for free and x if used)
void print_blocks_used();


// Test functions //

void test_general();
void test_free1();
void test_free2();
void test_malloc1();
void test_malloc2();
void test_realloc();
void header_test();


