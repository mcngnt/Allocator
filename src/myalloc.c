#include "myalloc.h"


#define MAX_SMALL 100


#define SIZE_BLK_SMALL 128 - sizeof(size_t)

#define SIZE_BLK_LARGE 1024

// Struct used to represent a block of memory
struct SmallBlock_s
{
	// The LSB of the header tells if the block is in use (LSB = 1) or not (LSB = 0)
	size_t header;
	// Body of the block
	char body[SIZE_BLK_SMALL];
};

typedef struct SmallBlock_s SmallBlock;


struct LargeBlock_s
{
	// The LSB of the header tells if the block is in use (LSB = 1) or not (LSB = 0)
	size_t header;
	// Size in bytes of the whole block (ie size of body + 2*sizeof(size_t))
	size_t size;
	// Body of the block
	char body[];
};


typedef struct LargeBlock_s LargeBlock;




// The memory
SmallBlock small_tab[MAX_SMALL];
SmallBlock* firstFreeBlock;
int isInit = 0;
LargeBlock* big_free = NULL;









// Returns 1 if the pointer is within the boundaries of memory and else 0
int is_memory_safe(void* ptr)
{
	return ptr >= (void*)((size_t*)small_tab - 1) && ptr < sbrk(0);
}








// Initialize memory headers by setting up the chained list of free blocks
void initialize_memory()
{
	big_free = (LargeBlock*)sbrk(SIZE_BLK_LARGE);
	if(big_free == (void*)-1)
	{
		printf("ERROR : no memory available on the heap.");
	}
	big_free->size = SIZE_BLK_LARGE;
	big_free->header = (size_t)NULL;

	firstFreeBlock = small_tab;
	for(int i = 0; i < MAX_SMALL - 1; ++i)
	{
		*(size_t*)(small_tab + i) = (size_t)(small_tab + i + 1);
	}
	*(size_t*)(small_tab+ MAX_SMALL - 1) = (size_t)NULL;
	isInit = 1;
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
		// Fullsize is the size of a block with a body fo size size
		size_t fullSize = size + 2*sizeof(size_t);

		// fullSizeMultSize is the size of a block multiple of sizeof(size_t) (for keeping blocks aligned)
		size_t fullSizeMultSize = ( fullSize / sizeof(size_t) ) * sizeof(size_t);
		if(fullSizeMultSize < fullSize)
		{
			fullSizeMultSize += sizeof(size_t);
		}

		LargeBlock* currentLargeBlock = big_free;
		LargeBlock* prevLargeBlock = NULL;

		// Looping over free large blocks to find one big enough to fit fullSizeMult
		while(currentLargeBlock != NULL)
		{
			if(currentLargeBlock->size >= fullSize)
			{
				// Here the block is small enough for keeping it intact
				if(currentLargeBlock->size < fullSize + SIZE_BLK_SMALL)
				{
					if(prevLargeBlock != NULL)
					{
						prevLargeBlock->header = currentLargeBlock->header;
					}
					else
					{
						big_free = (LargeBlock*)currentLargeBlock->header;
					}
					currentLargeBlock->header = 1;
					return (void*)currentLargeBlock->body;
				}
				else
				{
					// Else, the block is split into two parts to avoid fragmentation
					currentLargeBlock->size -= fullSizeMultSize;
					LargeBlock* newBlock = (LargeBlock*)((char*)currentLargeBlock + currentLargeBlock->size);
					newBlock->header = 1;
					newBlock->size = fullSizeMultSize;
					return (void*)newBlock->body;
				}
			}

			prevLargeBlock = currentLargeBlock;	
			currentLargeBlock = (LargeBlock*)currentLargeBlock->header;
		}
		
		// If no block large enough is found, memory is allocated on the heap

		LargeBlock* newBlock = sbrk(fullSizeMultSize);
		if(newBlock == (void*)-1)
		{
			printf("ERROR : no memory available on the heap.");
			return NULL;
		}
		newBlock->header = 1;
		newBlock->size = fullSizeMultSize;

		
		return (void*)newBlock->body;
		

	}

	// Now I deal with small blocks

	if(firstFreeBlock == NULL)
	{
		printf("ERROR : no memory for small blocks available.\n");
		return NULL;
	}

	SmallBlock* newBlock = firstFreeBlock;
	firstFreeBlock = (SmallBlock*)newBlock->header;
	newBlock->header = 1;


	// void* finalPtr = (void*)((size_t*)firstFreeBlock + 1);
	// *((size_t*)firstFreeBlock) += 1;
	// firstFreeBlock = (struct SmallBlock*)(*(size_t*)firstFreeBlock - 1);

	// return finalPtr;

	return newBlock->body;

}



// Frees the block associated to the pointer
void myFree(void* ptr)
{
	// Here, we check if the pointer points to something after the end of small_tab in memory
	if(!is_memory_safe(ptr))
	{
		printf("ERROR : incorrect address.\n");
		return;
	}

	
	// Small block case : 
	if(ptr < (void*)(small_tab + MAX_SMALL))
	{
		SmallBlock* currentSmallBlock = (SmallBlock*)((size_t*)ptr - 1);

		// In this case, the address does not points to the start of a block
		if((int)((char*)currentSmallBlock - (char*)small_tab ) % 128 != 0)
		{
			printf("ERROR : incorrect address.\n");
			return;
		}

		if(!(currentSmallBlock->header & 1))
		{
			printf("ERROR : referenced block not in use.\n");
			return;
		}

		// Set the header to points to the nest free block
		currentSmallBlock->header = (size_t)firstFreeBlock;
		firstFreeBlock = currentSmallBlock;
	}
	else
	{
		// I check if the address header of the block has a LSB of 1 (ie it is used)
		int isLargeBlock = *((size_t*)ptr - 2) & 1;
		LargeBlock* freeBlock = (LargeBlock*)((size_t*)ptr - 2);

		// Here I don't check if the address points to the start of the block (With the current global variables, I don't think there is any ways of doing it)

		if(isLargeBlock)
		{
			if(big_free == NULL)
			{
				big_free = freeBlock;
				big_free->header = (size_t)NULL;
				return;
			}


			LargeBlock* currentLargeBlock = big_free;
			LargeBlock* prevLargeBlock = NULL;

			// I loop over every free block to check if it is adjacent to the block that need to be freed (avoid memory fragmentation)
			while(currentLargeBlock != NULL)
			{
				if( ((char*)currentLargeBlock + currentLargeBlock->size ) == (char*)freeBlock )
				{
					currentLargeBlock->size += freeBlock->size;
					return;
				}


				if( ((char*)freeBlock + freeBlock->size) == (char*)currentLargeBlock )
				{
					freeBlock->size += currentLargeBlock->size;
					freeBlock->header = currentLargeBlock->header;
					if(prevLargeBlock != NULL)
					{
						prevLargeBlock->header = (size_t)freeBlock;
					}
					else
					{
						big_free = freeBlock;
					}
					return;
				}

				prevLargeBlock = currentLargeBlock;	
				currentLargeBlock = (LargeBlock*)currentLargeBlock->header;
			}

			// If no adjacent block found, simply add the block that nedd to be freed to the big_free list
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
	// Here, we check if the pointer points to something after the end of small_tab in memory
	if(!is_memory_safe(ptr))
	{
		printf("ERROR : incorrect address.\n");
		return NULL;
	}
	
	size_t bodySize = 0;

	if(ptr < (void*)(small_tab + MAX_SMALL) && *((size_t*)ptr - 1) & 1)
	{
		bodySize = SIZE_BLK_SMALL;
	}
	if(*((size_t*)ptr - 2) & 1)
	{
		bodySize = *((size_t*)ptr - 1) - 2*sizeof(size_t);
	}
	if(bodySize == 0)
	{
		printf("ERROR : incorrect address or block already in use.\n");
		return NULL;
	}


	// If the new pointer size is less than the previous pointer size, I try to avoid fragmentation and if not possible, I do nothing
	if(bodySize > size)
	{
		if(bodySize > SIZE_BLK_SMALL && bodySize > size + SIZE_BLK_SMALL + sizeof(size_t))
		{
			size_t fullSize = size + 2*sizeof(size_t);
			size_t fullSizeMultSize = ( fullSize / sizeof(size_t) ) * sizeof(size_t);
			if(fullSizeMultSize < fullSize)
				fullSizeMultSize += sizeof(size_t);

			// If the block is too large, I keep the first part of the block for the user (keeping intact the first part of its data)
			// and I free the other part

			LargeBlock* currentLargeBlock = (LargeBlock*)((size_t*)ptr - 2);
			LargeBlock* newBlock = (LargeBlock*)((char*)currentLargeBlock + fullSizeMultSize);
			newBlock->size = currentLargeBlock->size - fullSizeMultSize;
			newBlock->header = 1;
			currentLargeBlock->size = fullSizeMultSize;

			myFree((void*)newBlock->body);

			return (void*)currentLargeBlock->body;
		}
		return ptr;
	}


	// The pointer size is too small for the  neww content : I use a malloc-copy-free cycle

	void* newPtr = myMalloc(size);

	if(newPtr == NULL)
	{
		printf("ERROR : no memory available.");
		return NULL;
	}

	for (size_t i = 0; i < bodySize; i++)
	{
		*((char*)newPtr + i) = *((char*)ptr + i);
	}

	myFree(ptr);

	return newPtr;
	
}











// Prints the list of free large block on the heap with their size, address and header
void print_large_blocks_used()
{
	LargeBlock* currentLargeBlock = big_free;
	int counter = 0;

	printf("State of large blocks memory : \n");

	if(big_free == NULL)
	{
		printf("No large blocks in big_free.\n");
	}

	while(currentLargeBlock != NULL)
	{
		printf("Large block %d with address %p has a size of %d and a header of %p\n", counter, (void*)currentLargeBlock, (int)currentLargeBlock->size, (void*)currentLargeBlock->header);
		currentLargeBlock = (LargeBlock*)currentLargeBlock->header;
		counter++;
	}

	printf("End of state of large blocks memory.\n");
	
}

// Shows which blocks of memory are used ( o for free and x if used)
void print_small_blocks_used()
{
	printf("State of small blocks memory : \n");

	for(int i = 0; i < MAX_SMALL; ++i)
	{
		if(small_tab[i].header & 1)
		{
			printf("%d : x | ", i);
		}
		else
		{
			printf("%d : o | ", i);
		}
	}

	printf("\nEnd of state of small blocks memory.\n");
}


// Shows the content of a block by displaying the ascii representation of each of its bytes
void print_block_content(void* ptr)
{
	if(ptr < (void*)(small_tab + MAX_SMALL))
	{
		void* currentBlock = (void*)((size_t*)ptr - 1);
		int blockID = (int)( (char*)ptr - (char*)((size_t*)small_tab + 1) ) / 128;

		printf("Content of the %dth small block (address %p) : \n", blockID, currentBlock);
		for (unsigned int i = 0; i < SIZE_BLK_SMALL; ++i)
		{
			printf("%c/", *((char*)ptr + i));
		}
		printf("\nEnd of the %dth small block (address %p).\n", blockID, currentBlock);
	}
	else
	{
		LargeBlock* currentLargeBlock = (LargeBlock*)((size_t*)ptr - 2);

		printf("Content of large block with size %d (address %p) : \n", (int)currentLargeBlock->size, (void*)currentLargeBlock);
		for (unsigned int i = 0; i < currentLargeBlock->size - 2*sizeof(size_t); ++i)
		{
			printf("%c/", *((char*)currentLargeBlock->body + i));
		}
		printf("\nEnd of large block with size %d (address %p).\n", (int)currentLargeBlock->size, (void*)currentLargeBlock);
	}
}













// Returns int at a pointer address in memory after checking that the pointer is valid
int read_safe_int_small(void* ptr)
{
	if(!is_memory_safe(ptr))
	{
		printf("ERROR : incorrect address.\n");
		return 0;
	}

	if(ptr > (void*)(small_tab + MAX_SMALL))
	{
		printf("ERROR : read_safe_int_small can only read from small blocks.");
		return 0;
	}

	// Get the header of the block associated with the pointer, this time the pointer can point anywhere in the body
	size_t currentHeader = *((size_t*)( (char*)ptr - (char*)(( (char*)ptr - (char*)small_tab ) % 128) ));
	//                                                ^^^ calculates the number of bytes between the pointer and the start of the block
	if(!(currentHeader & 1))
	{
		printf("ERROR : referenced block not in use.\n");
		return 0;
	}

	return *((int*)ptr);

}

// Returns char at a pointer address in memory after checking that the pointer is valid
char read_safe_char_small(void* ptr)
{
	if(!is_memory_safe(ptr))
	{
		printf("ERROR : incorrect address.\n");
		return 0;
	}
	if(ptr > (void*)(small_tab + MAX_SMALL))
	{
		printf("ERROR : read_safe_char_small can only read from small blocks.");
		return 0;
	}
	// Get the header of the block associated with the pointer, this time the pointer can point anywhere in the body
	size_t currentHeader = *((size_t*)( (char*)ptr - (char*)(( (char*)ptr - (char*)small_tab ) % 128) ));
	//                                                ^^^ calculates the number of bytes between the pointer and the start of the block
	if(!(currentHeader & 1))
	{
		printf("ERROR : referenced block not in use.\n");
		return 0;
	}

	return *((char*)ptr);

}

// Writes int at a pointer address in memory after checking that the pointer is valid
void write_safe_int_small(void* ptr,int value)
{
	//                          Check if the (int) value is written inside the memory
	if(!is_memory_safe(ptr) && (void*)((int*)ptr + sizeof(int)) < (void*)(small_tab + MAX_SMALL))
	{
		printf("ERROR : incorrect address.\n");
		return;
	}
	if(ptr > (void*)(small_tab + MAX_SMALL))
	{
		printf("ERROR : write_safe_int_small can only write in small blocks.");
		return;
	}
	// Get the header of the block associated with the pointer, this time the pointer can point anywhere in the body
	size_t currentHeader = *((size_t*)( (char*)ptr - (char*)(( (char*)ptr - (char*)small_tab ) % 128) ));
	//                                                ^^^ calculates the number of bytes between the pointer and the start of the block
	if(!(currentHeader & 1))
	{
		printf("ERROR : referenced block not in use.\n");
		return;
	}

	*((int*)ptr) = value;

}

// Writes char at a pointer address in memory after checking that the pointer is valid
void write_safe_char_small(void* ptr,char value)
{
	//                          Check if the (char) value is written inside the memory
	if(!is_memory_safe(ptr) && (void*)((int*)ptr + sizeof(char)) < (void*)(small_tab + MAX_SMALL))
	{
		printf("ERROR : incorrect address.\n");
		return;
	}
	if(ptr > (void*)(small_tab + MAX_SMALL))
	{
		printf("ERROR : write_safe_char_small can only write in small blocks.");
		return;
	}
	// Get the header of the block associated with the pointer, this time the pointer can point anywhere in the body
	size_t currentHeader = *((size_t*)( (char*)ptr - (char*)(( (char*)ptr - (char*)small_tab ) % 128) ));
	//                                                ^^^ calculates the number of bytes between the pointer and the start of the block
	if(!(currentHeader & 1))
	{
		printf("ERROR : referenced block not in use.\n");
		return;
	}

	*((char*)ptr) = value;

}













void test_large_block1()
{
	print_small_blocks_used();
	print_large_blocks_used();

	char* tab = myMalloc(70 * sizeof(char));

	for(int i = 0 ; i < 58; i++)
	{
		tab[i] = (char)('A'+i);
	}

	printf("Malloc array of 70 chars and has written the letters of the alphabet\n");

	print_block_content(tab);

	print_large_blocks_used();

	print_small_blocks_used();

	char* tab2 = myRealloc(tab, sizeof(uint64_t) * 100);

	printf("Realloc the char array to a 100 elements array of 64 bits unsigned integers\n");

	print_block_content(tab2);

	print_large_blocks_used();

	print_small_blocks_used();

	myFree(tab2);

	printf("Free the unsigned int array\n");

	print_large_blocks_used();
}

void test_large_block2()
{
	print_small_blocks_used();
	print_large_blocks_used();

	char* tab = myMalloc(100 * sizeof(char));

	printf("Malloc array of 100 chars\n");

	print_small_blocks_used();

	uint64_t* tab2 = myMalloc(100 * sizeof(uint64_t));

	printf("Malloc array of 100 uint64_t\n");

	print_large_blocks_used();

	int* tab3 = myMalloc(300 * sizeof(int));

	printf("Malloc array of 300 int\n");

	print_large_blocks_used();

	myFree(tab3);

	printf("Free array of 300 int\n");

	print_large_blocks_used();

	for (size_t i = 0; i < 100; i++)
	{
		tab2[i] = i*i;
	}

	printf("Write squares in uint_64 array\n");

	print_block_content(tab2);

	char* tab4 = myMalloc(128 * sizeof(char));

	printf("Malloc array of 128 chars\n");

	print_large_blocks_used();


	float* tab5 = myRealloc(tab2, 10*sizeof(float));

	printf("Realloc array of 100 uint64 to 10 float\n");

	print_block_content(tab5);
	print_large_blocks_used();

	myFree(tab);
	printf("Free array of 100 char\n");
	print_large_blocks_used();


	myFree(tab5);
	printf("Free array of 10 float\n");
	print_large_blocks_used();

	myFree(tab4);
	printf("Free array of 128 char\n");
	print_large_blocks_used();

	print_small_blocks_used();

	printf("Error because i try to free memory that is not in the heap yet :\n");
	myFree(tab5 + 3000);


	printf("small_tab : %p break address : %p diff : %ld", (void*)small_tab, sbrk(0), sbrk(0) - (void*)small_tab);

	
}




void test_general()
{
	printf("Start of memory : %p\n", (void*)small_tab);

	// Alocating an int[5]
	int* ptr = (int*)( myMalloc(5*sizeof(int)) );
	printf("Just allocated memory for a 5 int array with body pointer : %p\n", (void*)ptr);

	*(ptr) = 12000;
	*(ptr + 1) = -100;
	*(ptr + 2) = 333;
	*(ptr + 3) = 0;
	*(ptr + 4) = -3;

	printf("Setting up the value of each element of the array.\n");


	write_safe_int_small(ptr + 1, 42);
	printf("Just wrote int %d at the address : %p\n", 42, (void*)(ptr + 1));



	print_block_content(ptr);

	for (int i = 0; i < 4; ++i)
	{
		// I can print content of the array
		read_safe_int_small(ptr + i);
		printf("Just read int %d at the address : %p\n", *((int*)(ptr + i)), (void*)(ptr + i));
		read_safe_char_small(ptr + i);
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
		read_safe_int_small(ptr + i);
		read_safe_char_small(ptr + i);
	}

	write_safe_int_small(ptr1, -23987);
	printf("Just wrote int %d at the address : %p\n", -23987, (void*)(ptr1));

	print_block_content(ptr1);

	char* ptr2 = (char*)( myRealloc((void*)ptr1, sizeof(char)) );
	printf("Just reallocated memory with body pointer : %p \n", (void*)ptr2);


	// I reallocate the block associated with ptr1 and use it to store a char @
	write_safe_char_small(ptr2, '@');
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
		write_safe_int_small(ptr, -i*i*i);
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
	printf("Memory reset.\n");

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
    
    write_safe_int_small(ptr1, 6789);
    printf("Just wrote int %d at the address : %p\n", 6789, (void*)(ptr1));

    print_block_content(ptr1);


    ptr1 = (int *)myRealloc(ptr1,10 * sizeof(int));
    printf("Just reallocated memory with body pointer : %p \n", (void*)ptr1);


    write_safe_int_small(ptr1, -5);
    printf("Just wrote int %d at the address : %p\n", -5, (void*)(ptr1));
    write_safe_int_small(ptr1 + 1, 20);
    printf("Just wrote int %d at the address : %p\n", 20, (void*)(ptr1+1));
    write_safe_int_small(ptr1 + 2, -35);
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

	write_safe_int_small(test, -177);
	printf("Just wrote int %d at the address : %p\n", -177, (void*)(test));

	print_small_blocks_used();

	char* test2 = myMalloc(sizeof(char)*10);
	printf("Just allocated memory with body pointer : %p\n", (void*)test2);

	write_safe_int_small(test2, 'E');
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
			addresses[i] = myMalloc(sizeof(int)*20);
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
			addresses[i] = malloc(sizeof(int)*20);
		}
		for (size_t i = 0; i < MAX_SMALL; ++i)
		{
			free(addresses[i]);
		}
	}
	end_time = clock();
    
    printf("Time taken stdlib allocator: %f seconds\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);

}