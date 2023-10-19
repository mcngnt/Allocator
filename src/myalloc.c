#include "myalloc.h"


#define MAX_SMALL 100


#define SIZE_BLK_SMALL 128 - sizeof(size_t)

#define SIZE_BLK_LARGE 1024

// Struct used to represent a block of memory
struct SmallBlock
{
	// The LSB of the header tells if the block is in use (LSB = 1) or not (LSB = 0)
	size_t header;
	// Body of the block
	char body[SIZE_BLK_SMALL];
};

typedef struct LargeBlock_s LargeBlock;

struct LargeBlock_s
{
	// The LSB of the header tells if the block is in use (LSB = 1) or not (LSB = 0)
	size_t header;
	size_t size;
	// Body of the block
	char body[];
};



// The memory
struct SmallBlock small_tab[MAX_SMALL];
struct SmallBlock* firstFreeBlock;
int isInit = 0;
LargeBlock* big_free = NULL;


// Initialize memory headers by setting up the chained list of free blocks
void initialize_memory()
{
	big_free = NULL;
	big_free = (LargeBlock*)sbrk(SIZE_BLK_LARGE);
	big_free->size = SIZE_BLK_LARGE;
	big_free->header = 0;

	firstFreeBlock = small_tab;
	for(int i = 0; i < MAX_SMALL - 1; ++i)
	{
		*(size_t*)(small_tab + i) = (size_t)(small_tab + i + 1);
	}
	*(size_t*)(small_tab+ MAX_SMALL - 1) = (size_t)NULL;
	isInit = 1;
}

// Returns 1 if the pointer is within the boundaries of memory and else 0
int is_memory_safe(void* ptr)
{
	return ptr >= (void*)((size_t*)small_tab - 1);
}




// Returns a pointer to the body of a memory block
void* myMalloc(size_t size)
{
	if(!isInit)
	{
		initialize_memory();
	}

	if(size > SIZE_BLK_SMALL)
	{
		size_t fullSize = size + 2*sizeof(size_t);

		LargeBlock* currentLargeBlock = big_free;
		LargeBlock* prevLargeBlock = NULL;

		while(currentLargeBlock != NULL)
		{
			if(currentLargeBlock->size >= size)
			{
				if(currentLargeBlock->size < size + SIZE_BLK_SMALL)
				{
					if(prevLargeBlock != NULL)
					{
						prevLargeBlock->header = currentLargeBlock->header;
					}
					else
					{
						big_free = (LargeBlock*)currentLargeBlock->header;
					}
					currentLargeBlock->header += 1;
					return (void*)currentLargeBlock->body;
				}
				else
				{
					int k = ( fullSize / sizeof(size_t) ) * sizeof(size_t);
					if(k < fullSize)
					{
						k += sizeof(size_t);
					}
					
					currentLargeBlock->size -= k;
					LargeBlock* newBlock = (LargeBlock*)(currentLargeBlock->body + currentLargeBlock->size);
					newBlock->header = 1;
					newBlock->size = k - 2 * sizeof(size_t);
					return (void*)newBlock->body;
				}
			}

			prevLargeBlock = currentLargeBlock;	
			currentLargeBlock = (LargeBlock*)currentLargeBlock->header;
		}
		

		LargeBlock* newBlock = sbrk(fullSize);
		newBlock->header += 1;
		newBlock->size = size;


		return (void*)newBlock->body;
		

	}

	if(firstFreeBlock == NULL)
	{
		printf("ERROR : No memory available.\n");
		return NULL;
	}

	void* finalPtr = (void*)((size_t*)firstFreeBlock + 1);
	*((size_t*)firstFreeBlock) += 1;
	firstFreeBlock = (struct SmallBlock*)(*(size_t*)firstFreeBlock - 1);

	return finalPtr;

}



// Frees the block associated to the pointer
void myFree(void* ptr)
{
	// Here, we check if the pointer points to the start of the body of a block
	if(!is_memory_safe(ptr))
	{
		printf("ERROR : incorrect address.\n");
		return;
	}
	

	if(ptr < (void*)(small_tab + MAX_SMALL))
	{
		if((int)((char*)((size_t*)ptr - 1) - (char*)small_tab ) % 128 != 0)
		{
			printf("ERROR : incorrect address.\n");
			return;
		}

		// Get the header of the current block
		size_t currentHeader = *((size_t*)ptr - 1);
		if(currentHeader % 2 == 0)
		{
			printf("ERROR : referenced block not in use.\n");
			return;
		}

		// Clears the LSB of the header of the block
		*((size_t*)ptr - 1) = (size_t)firstFreeBlock;
		firstFreeBlock = (struct SmallBlock*)((size_t*)ptr - 1);
	}
	else
	{
		int isLargeBlock = *((size_t*)ptr - 2) & 1;
		LargeBlock* freeBlock = (LargeBlock*)((size_t*)ptr - 2);
		if(isLargeBlock)
		{
			if(big_free == NULL)
			{
				big_free = freeBlock;
				big_free->header = (size_t)NULL;
				return;
			}

			if( ((char*)big_free + 2*sizeof(size_t) + big_free->size ) == (char*)freeBlock )
			{
				big_free->size += freeBlock->size + 2 * sizeof(size_t);
				return;
			}

			if( ((char*)freeBlock +  + 2*sizeof(size_t) + freeBlock->size) == (char*)big_free )
			{
				freeBlock->size += big_free->size + 2 * sizeof(size_t);
				freeBlock->header = big_free->header;
				big_free = freeBlock;
				return;
			}
			
			freeBlock->header = (size_t)big_free;
			big_free = freeBlock;
		}
		else
		{
			printf("ERROR : referenced block not in use.\n");
			return;
		}
	}

}



// Frees the block associated to the pointer and reallocate it for new data
void* myRealloc(void* ptr, size_t size)
{
	if(!is_memory_safe(ptr))
	{
		printf("ERROR : incorrect address.\n");
		return NULL;
	}
	
	size_t blockSize = 0;

	if(ptr < (void*)(small_tab + MAX_SMALL) && *((size_t*)ptr - 1) & 1)
	{
		blockSize = SIZE_BLK_SMALL;
	}
	if(*((size_t*)ptr - 2) & 1)
	{
		blockSize = *((size_t*)ptr - 1);
	}
	if(blockSize == 0)
	{
		printf("ERROR : incorrect address or block already in use.\n");
		return NULL;
	}

	if(blockSize > size)
	{
		return ptr;
	}

	void* newPtr = myMalloc(size);

	for (size_t i = 0; i < blockSize; i++)
	{
		*((char*)newPtr + i) = *((char*)ptr + i);
	}

	myFree(ptr);

	return newPtr;
	
}

void print_free_large_blocks()
{
	LargeBlock* currentLargeBlock = big_free;
	LargeBlock* prevLargeBlock = NULL;
	int counter = 0;

	printf("State of large blocks memory : \n");

	if(big_free == NULL)
	{
		printf("No large blocks in big_free.\n");
	}

	while(currentLargeBlock != NULL)
	{
		printf("Large block %d with address %p has a size of %d and a header of %p\n", counter, (void*)currentLargeBlock, (int)currentLargeBlock->size, (void*)currentLargeBlock->header);

		prevLargeBlock = currentLargeBlock;
		currentLargeBlock = (LargeBlock*)currentLargeBlock->header;
		counter++;
	}

	printf("End of state of large blocks memory.\n");
	
}


void test_large_block()
{
	print_small_blocks_used();
	printf("Current value of big_free (not initialized) : %p\n", (void*)big_free);

	char* tab = myMalloc(30 * sizeof(char));

	for(int i = 0 ; i < 26; i++)
	{
		tab[i] = (char)('A'+i);
	}

	printf("Malloc array of 30 chars and has written the letters of the alphabet\n");

	print_block_content(tab);

	print_free_large_blocks();

	print_small_blocks_used();

	char* tab2 = myRealloc(tab, sizeof(uint64_t) * 100);

	printf("Realloc the char array to a 100 elements array of 64 bits unsigned integers\n");

	print_block_content(tab2);

	print_free_large_blocks();

	print_small_blocks_used();

	myFree(tab2);

	printf("Free the unisnged int array\n");

	print_free_large_blocks();
}


void speed_test(size_t testNB)
{
	printf("Speed test for %u tests : \n", (unsigned int)testNB);
	int* addresses[MAX_SMALL];
	clock_t start_time, end_time;

	start_time = clock();

	for (size_t nb = 0; nb < testNB; ++nb)
	{
		for (size_t i = 0; i < MAX_SMALL; ++i)
		{
			addresses[i] = myMalloc(sizeof(int)*10);
		}
		for (size_t i = 0; i < MAX_SMALL; ++i)
		{
			myFree(addresses[i]);
		}
	}
	end_time = clock();
    
    printf("Time taken homemade allocator: %f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

    start_time = clock();

	for (size_t nb = 0; nb < testNB; ++nb)
	{
		for (size_t i = 0; i < MAX_SMALL; ++i)
		{
			addresses[i] = malloc(sizeof(int)*10);
		}
		for (size_t i = 0; i < MAX_SMALL; ++i)
		{
			free(addresses[i]);
		}
	}
	end_time = clock();
    
    printf("Time taken stdlib allocator: %f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

}

// Returns int at a pointer address in memory after checking that the pointer is valid
int read_safe_int(void* ptr)
{
	if(!is_memory_safe(ptr))
	{
		printf("ERROR : incorrect address.\n");
		return 0;
	}
	// Get the header of the block associated with the pointer, this time the pointer can point anywhere in the body
	size_t currentHeader = *((size_t*)( (char*)ptr - (char*)(( (char*)ptr - (char*)small_tab ) % 128) ));
	//                                                ^^^ calculates the number of bytes between the pointer and the start of the block
	if(currentHeader % 2 == 0)
	{
		printf("ERROR : referenced block not in use.\n");
		return 0;
	}

	return *((int*)ptr);

}

// Returns char at a pointer address in memory after checking that the pointer is valid
char read_safe_char(void* ptr)
{
	if(!is_memory_safe(ptr))
	{
		printf("ERROR : incorrect address.\n");
		return 0;
	}
	// Get the header of the block associated with the pointer, this time the pointer can point anywhere in the body
	size_t currentHeader = *((size_t*)( (char*)ptr - (char*)(( (char*)ptr - (char*)small_tab ) % 128) ));
	//                                                ^^^ calculates the number of bytes between the pointer and the start of the block
	if(currentHeader % 2 == 0)
	{
		printf("ERROR : referenced block not in use.\n");
		return 0;
	}

	return *((char*)ptr);

}

// Writes int at a pointer address in memory after checking that the pointer is valid
void write_safe_int(void* ptr,int value)
{
	//                          Check if the (int) value is written inside the memory
	if(!is_memory_safe(ptr) && (void*)((int*)ptr + sizeof(int)) < (void*)(small_tab + MAX_SMALL))
	{
		printf("ERROR : incorrect address.\n");
		return;
	}
	// Get the header of the block associated with the pointer, this time the pointer can point anywhere in the body
	size_t currentHeader = *((size_t*)( (char*)ptr - (char*)(( (char*)ptr - (char*)small_tab ) % 128) ));
	//                                                ^^^ calculates the number of bytes between the pointer and the start of the block
	if(currentHeader % 2 == 0)
	{
		printf("ERROR : referenced block not in use.\n");
		return;
	}

	*((int*)ptr) = value;

}

// Writes char at a pointer address in memory after checking that the pointer is valid
void write_safe_char(void* ptr,char value)
{
	//                          Check if the (char) value is written inside the memory
	if(!is_memory_safe(ptr) && (void*)((int*)ptr + sizeof(char)) < (void*)(small_tab + MAX_SMALL))
	{
		printf("ERROR : incorrect address.\n");
		return;
	}
	// Get the header of the block associated with the pointer, this time the pointer can point anywhere in the body
	size_t currentHeader = *((size_t*)( (char*)ptr - (char*)(( (char*)ptr - (char*)small_tab ) % 128) ));
	//                                                ^^^ calculates the number of bytes between the pointer and the start of the block
	if(currentHeader % 2 == 0)
	{
		printf("ERROR : referenced block not in use.\n");
		return;
	}

	*((char*)ptr) = value;

}



// Shows which blocks of memory are used ( o for free and x if used)
void print_small_blocks_used()
{
	printf("State of small blocks memory : \n");

	for(int i = 0; i < MAX_SMALL; ++i)
	{
		if(small_tab[i].header % 2 == 0)
		{
			printf("%d : o | ", i);
		}
		else
		{
			printf("%d : x | ", i);
		}
	}

	printf("\nEnd of state of small blocks memory.\n");
}


// Shows the content of a block by displaying the ascii representation of each of its bytes
void print_block_content(void* ptr)
{
	// char* currentBlock = (char*)( (size_t*)( (char*)ptr - (char*)(( (char*)ptr - (char*)small_tab ) % 128) ) + 1 );
	
	// int blockID = (int)( (char*)currentBlock - (char*)((size_t*)small_tab + 1) ) / 128;

	if(ptr < (void*)(small_tab + MAX_SMALL))
	{
		void* currentBlock = (void*)((size_t*)ptr - 1);
		int blockID = (int)( (char*)ptr - (char*)((size_t*)small_tab + 1) ) / 128;

		printf("Content of the %dth small block (address %p) : \n", blockID, currentBlock);
		for (unsigned int i = 0; i < SIZE_BLK_SMALL; ++i)
		{
			printf("%c/", *((char*)ptr + i));
		}
		printf("\nEnd of the %dth small block (address %p) .\n", blockID, currentBlock);
	}
	else
	{
		LargeBlock* currentLargeBlock = (LargeBlock*)((size_t*)ptr - 2);

		printf("Content of large block (address %p) : \n", (void*)currentLargeBlock);
		for (unsigned int i = 0; i < currentLargeBlock->size; ++i)
		{
			printf("%c/", *((char*)ptr + i));
		}
		printf("\nEnd of large block (address %p) .\n", (void*)currentLargeBlock);
	}
}


void test_general()
{
	printf("Start of memory : %p\n", (void*)small_tab);

	// Alocating an int[5]
	int* ptr = (int*)( myMalloc(5*sizeof(int)) );
	printf("Just allocated memory with body pointer : %p\n", (void*)ptr);

	*(ptr) = 12000;
	*(ptr + 1) = -100;
	*(ptr + 2) = 333;
	*(ptr + 3) = 0;
	*(ptr + 4) = -3;


	write_safe_int(ptr + 1, 42);
	printf("Just wrote int %d at the address : %p\n", 42, (void*)(ptr + 1));



	print_block_content(ptr);

	for (int i = 0; i < 4; ++i)
	{
		// I can print content of the array
		read_safe_int(ptr + i);
		printf("Just read int %d at the address : %p\n", *((int*)(ptr + i)), (void*)(ptr + i));
		read_safe_char(ptr + i);
		printf("Just read char %c at the address : %p\n", *((char*)(ptr + i)), (void*)(ptr + i));
	}

	int* ptr1 = (int*)( myMalloc(sizeof(int)) );
	printf("Just allocated memory with body pointer : %p\n", (void*)ptr1);



	myFree((void*)ptr);
	printf("Just freed block with address : %p\n", (void*)ptr);

	printf("Block is already freed, 8 errors : \n");

	for (int i = 0; i < 4; ++i)
	{
		// Error : the block was freed
		read_safe_int(ptr + i);
		read_safe_char(ptr + i);
	}

	write_safe_int(ptr1, -23987);
	printf("Just wrote int %d at the address : %p\n", -23987, (void*)(ptr1));

	print_block_content(ptr1);

	char* ptr2 = (char*)( myRealloc((void*)ptr1, sizeof(char)) );
	printf("Just reallocated memory with body pointer : %p \n", (void*)ptr2);


	// I reallocate the block associated with ptr1 and use it to store a char @
	write_safe_char(ptr2, '@');
	printf("Just wrote int %c at the address : %p\n", '@', (void*)ptr2);


	// @ can be seen at the first byte of the block 1
	print_block_content(ptr2);

	// Only 1 is used (0)
	print_small_blocks_used();

	myFree((void*)ptr2);

	printf("Just freed block with address : %p\n", (void*)ptr2);

	print_small_blocks_used();
}

void test_free1()
{
	int* ptr = (int*)( myMalloc(5*sizeof(int)) );
	printf("Just allocated memory with body pointer : %p\n", (void*)ptr);
	char* ptr1 = (char*)( myMalloc(2*sizeof(char)) );
	printf("Just allocated memory with body pointer : %p\n", (void*)ptr1);
	myFree(ptr1);
	printf("Just freed block with address : %p\n", (void*)ptr1);
	size_t* ptr2 = (size_t*)( myMalloc(sizeof(size_t)) );
	printf("Just allocated memory with body pointer : %p\n", (void*)ptr2);
	long* ptr3 = (long*)( myMalloc(3*sizeof(long)) );
	printf("Just allocated memory with body pointer : %p\n", (void*)ptr3);

	// Only three blocks used
	print_small_blocks_used();

	myFree(ptr);
	printf("Just freed block with address : %p\n", (void*)ptr);
	myFree(ptr3);
	printf("Just freed block with address : %p\n", (void*)ptr3);
	myFree(ptr2);
	printf("Just freed block with address : %p\n", (void*)ptr2);

	// No block used
	print_small_blocks_used();
}

void test_free2()
{
	char* ptr = (char*)( myMalloc(5*sizeof(char)) );
	printf("Just allocated memory with body pointer : %p\n", (void*)ptr);

	// One block used
	print_small_blocks_used();

	myFree(ptr);
	printf("Just freed block with address : %p\n", (void*)ptr);
	// Error : already feed
	printf("Error because the block was already freed : \n");
	myFree(ptr);
	printf("Just freed block with address : %p\n", (void*)ptr);

	// No block used
	print_small_blocks_used();
}

void test_malloc1()
{
	// Here, I try to do a memory overflow

	for (int i = 0; i < MAX_SMALL + 5; ++i)
	{
		// OK until i >= MAX_SMALL where there is no memory left
		long* ptr = (long*)(myMalloc(sizeof(long)));
		write_safe_int(ptr, -i*i*i);
		if(i < MAX_SMALL)
		{
			printf("Just allocated memory with body pointer : %p\n", (void*)ptr);
			printf("Just wrote int %d at the address : %p\n", -i*i*i, (void*)(ptr));
		}
		else
		{
			printf("Error because the memory is full.\n");
		}
	}

	print_small_blocks_used();

	initialize_memory();
	printf("Memory all cleared\n");

	print_small_blocks_used();


}

void test_malloc2()
{
	printf("Error because trying to store a 128 int array in a smallblock : \n");
	int* ptr = (int*)( myMalloc(128*sizeof(int)) );
	print_small_blocks_used();
	printf("Error because the int array was never allocated : \n");
	myFree(ptr);
}

void test_realloc()
{
	int* ptr1 = (int*)myMalloc(sizeof(int));
	printf("Just allocated memory with body pointer : %p\n", (void*)ptr1);
    int* ptr2 = (int*)myMalloc(sizeof(int));
    printf("Just allocated memory with body pointer : %p\n", (void*)ptr2);
    
    write_safe_int(ptr1, 6789);
    printf("Just wrote int %d at the address : %p\n", 6789, (void*)(ptr1));

    print_block_content(ptr1);


    ptr1 = (int *)myRealloc(ptr1,10 * sizeof(int));
    printf("Just reallocated memory with body pointer : %p \n", (void*)ptr1);


    write_safe_int(ptr1, -5);
    printf("Just wrote int %d at the address : %p\n", -5, (void*)(ptr1));
    write_safe_int(ptr1 + 1, 20);
    printf("Just wrote int %d at the address : %p\n", 20, (void*)(ptr1+1));
    write_safe_int(ptr1 + 2, -35);
    printf("Just wrote int %d at the address : %p\n", -35, (void*)(ptr1+2));

    print_block_content(ptr1);

    myFree(ptr1);
    printf("Just freed block with address : %p\n", (void*)ptr1);
    myFree(ptr2);
    printf("Just freed block with address : %p\n", (void*)ptr2);

    print_block_content(ptr1);
}


void test_header()
{
	print_small_blocks_used();

	int* test = myMalloc(sizeof(int));
	printf("Just allocated memory with body pointer : %p\n", (void*)test);

	write_safe_int(test, -177);
	printf("Just wrote int %d at the address : %p\n", -177, (void*)(test));

	print_small_blocks_used();

	char* test2 = myMalloc(sizeof(char)*10);
	printf("Just allocated memory with body pointer : %p\n", (void*)test2);

	write_safe_int(test2, 'E');
	printf("Just wrote char %c at the address : %p\n", 'E', (void*)(test2));


	print_small_blocks_used();


	printf("Small tab address : %p\n", (void*)small_tab);
	printf("First free block address : %p\n", (void*)firstFreeBlock);
	printf("Number of bytes between small_tab and the first free block : %d\n", (int)((char*)firstFreeBlock - (char*)small_tab));
	

	myFree(test);
	printf("Just freed block with address : %p\n", (void*)test);

	printf("First free block address : %p\n", (void*)firstFreeBlock);

	printf("Headers of memory : \n");

	for (size_t i = 0; i < MAX_SMALL; i++)
	{
		printf("Header containing address %p at the index %d\n", (void*)(*(size_t*)(small_tab + i)), (int)i);
	}

	myFree(test2);
	printf("Just freed block with address : %p\n", (void*)test2);
	
}

