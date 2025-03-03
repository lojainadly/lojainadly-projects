///////////////////////////////////////////////////////////////////////////////
// Main File:        p3Heap.c
// This File:        p3Heap.c
// Other Files:      p3Heap.o, p3Heap.h, Makefile, /tests 
// Semester:         CS 354 Lecture 001      SPRING 2024
// Instructor:       deppeler
// 
// Author:           Lojain Adly
// Email:            ladly@wisc.edu
// CS Login:         lojain
/////////////////////////////////////////////////////////////////////////////
// Copyright 2020-2024 Deb Deppeler based on work by Jim Skrentny
/////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include "p3Heap.h"

/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block.
 */
typedef struct blockHeader {           

	/*
	 * 1) The size of each heap block must be a multiple of 8
	 * 2) heap blocks have blockHeaders that contain size and status bits
	 * 3) free heap block contain a footer, but we can use the blockHeader 
	 *.
	 * All heap blocks have a blockHeader with size and status
	 * Free heap blocks have a blockHeader as its footer with size only
	 *
	 * Status is stored using the two least significant bits.
	 *   Bit0 => least significant bit, last bit
	 *   Bit0 == 0 => free block
	 *   Bit0 == 1 => allocated block
	 *
	 *   Bit1 => second last bit 
	 *   Bit1 == 0 => previous block is free
	 *   Bit1 == 1 => previous block is allocated
	 * 
	 * Start Heap: 
	 *  The blockHeader for the first block of the heap is after skip 4 bytes.
	 *  This ensures alignment requirements can be met.
	 * 
	 * End Mark: 
	 *  The end of the available memory is indicated using a size_status of 1.
	 * 
	 * Examples:
	 * 
	 * 1. Allocated block of size 24 bytes:
	 *    Allocated Block Header:
	 *      If the previous block is free      p-bit=0 size_status would be 25
	 *      If the previous block is allocated p-bit=1 size_status would be 27
	 * 
	 * 2. Free block of size 24 bytes:
	 *    Free Block Header:
	 *      If the previous block is free      p-bit=0 size_status would be 24
	 *      If the previous block is allocated p-bit=1 size_status would be 26
	 *    Free Block Footer:
	 *      size_status should be 24
	 */
	int size_status;

} blockHeader;         

/* Global variable - DO NOT CHANGE NAME or TYPE. 
 * It must point to the first block in the heap and is set by init_heap()
 * i.e., the block at the lowest address.
 */
blockHeader *heap_start = NULL;     

/* Size of heap allocation padded to round to nearest page size.
 */
int alloc_size;

/*
 * Additional global variables may be added as needed below
 * TODO: add global variables needed by your function
 */

/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block (payload) on success.
 * Returns NULL on failure.
 *
 * This function must:
 * - Check size - Return NULL if size < 1 
 * - Determine block size rounding up to a multiple of 8 
 *   and possibly adding padding as a result.
 *
 * - Use BEST-FIT PLACEMENT POLICY to chose a free block
 *
 * - If the BEST-FIT block that is found is exact size match
 *   - 1. Update all heap blocks as needed for any affected blocks
 *   - 2. Return the address of the allocated block payload
 *
 * - If the BEST-FIT block that is found is large enough to split 
 *   - 1. SPLIT the free block into two valid heap blocks:
 *         1. an allocated block
 *         2. a free block
 *         NOTE: both blocks must meet heap block requirements 
 *       - Update all heap block header(s) and footer(s) 
 *              as needed for any affected blocks.
 *   - 2. Return the address of the allocated block payload
 *
 *   Return if NULL unable to find and allocate block for required size
 *
 * Note: payload address that is returned is NOT the address of the
 *       block header.  It is the address of the start of the 
 *       available memory for the requesterr.
 *
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* balloc(int size) {     
	//TODO: Your code goes in here.
	blockHeader *footer = NULL;
	//create bestFit blockheader and abit mask, pbit mask, blocksize, and true size int
	blockHeader *bestFit = NULL;
	int blockSize;
	int trueSize;
	int abit_mask = 1;
	int pbit_mask = 2;
	int sizeMask = 4088;


	if (size < 1)
		return NULL;

	//check if padding needs to be made
	if ((size + sizeof(blockHeader*)) % 8 != 0){
		blockSize = size + sizeof(blockHeader*) + (8-(size + sizeof(blockHeader*)) % 8);
	} else {
		blockSize = size + sizeof(blockHeader*);
	}

	//start at the first block allocated ( as per best fit algo)
	//ptr to the start of the heap to start algo
	blockHeader* newBlock = heap_start;

	//run algo as long as the end of mem is not reached	
	while (newBlock->size_status != 1){
		//make truesize the new block at every iteration of loop
		trueSize = (newBlock->size_status & sizeMask);

		//check if current header is available to allocate to
		if ((newBlock->size_status & abit_mask) == 0 ){

			//check if truesize is equal to blockSize
			if (trueSize == blockSize) {
				//if so, add new header and return pointer to payload
				//also update p bit of next block (show that this one is now allocated)
				newBlock->size_status += 1;
				blockHeader* nextBlock = (void*) newBlock + trueSize;

				//update p bit of next block to signify this one is allocated
				//CHECK IF THIS IS THE LAST BLOCK
				if(nextBlock->size_status != 1){
					nextBlock->size_status += 2; 
				}
				return (void*) (newBlock) + sizeof(blockHeader*);
			}

			// when block size is not equal to the payload, but is greater, then save block in best fit pointer
			//(return +4 for payload) and increase address by size
			if(trueSize > blockSize){

				//if the bestFit IS NULL, then we just make the newBlock that is there the bestFit.
				if (bestFit == NULL){
					bestFit = (void*) (newBlock);
					//note the best fit size from the block that we mark
					bestFit->size_status = newBlock->size_status;
				}
				//else, this means that we need to check the new block against the current bestFit block
				else{
					if (newBlock->size_status < bestFit->size_status){
						//then we make bestFit this new block. Otherwise do not change bestFit
						bestFit = (void*) (newBlock);
						bestFit->size_status = newBlock->size_status;
					}
				}
				//increment to next heap address available
				newBlock = (void*) (newBlock) + trueSize;
				continue;

			}
			// if it is less, thjen we continue through the loop adn add to address
			else{
				newBlock = (void*) (newBlock) + trueSize;
				continue;
			}	
		}
		//if block is already allocated, then we should add size and check next block in loop
		if ((newBlock->size_status & abit_mask) == 1){
			//check if the next block has a pbit
			blockHeader *testPBit = (void*) (newBlock) + trueSize;
			//if it does not, then add the pbit to signify that their previous block is allocated
			if ((testPBit->size_status & pbit_mask) == 0) {
				//CHECK THAT IT IS NOT THE END OF HEAP
				if (testPBit->size_status != 1){
					testPBit->size_status += 2;
				}
			}
			//move to next header
			newBlock = (void*) (newBlock) + trueSize;

		}
	}
	// splitting function
	if (bestFit == NULL){
		return NULL;
	}
	//checks if split is applicable 
	trueSize = (bestFit->size_status & sizeMask);
	if (trueSize - blockSize >= 8){
		//if it is, we can split the block (add a new header at the address of the bestFit + blockSize)
		//change the size of bestFit, and change the size of the remaining free block
		//dont forget to add a footer to each free block

		bestFit->size_status = blockSize;
		blockHeader *splitBlock = (void*)(bestFit) + blockSize;
		//change the split block size to what is left from split, and add 2 to p-bit of split block
		splitBlock->size_status = trueSize - blockSize + 2 ;

		//add footer to end of splitBlock to show its size 
		footer = (void*) (splitBlock) + splitBlock->size_status - 6;
		footer->size_status = splitBlock->size_status-2;

	}

	//check if this block is in heap start. if so, add 3 to size_status
	if ((void*)bestFit == heap_start){
		bestFit->size_status += 3;	 
	}else{
		bestFit->size_status += 1;
	}	
	return (void*) bestFit + 4;
} 

/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - Update header(s) and footer as needed.
 *
 * If free results in two or more adjacent free blocks,
 * they will be immediately coalesced into one larger free block.
 * so free blocks require a footer (blockHeader works) to store the size
 *
 * TIP: work on getting immediate coalescing to work after your code 
 *      can pass the tests in partA and partB of tests/ directory.
 *      Submit code that passes partA and partB to Canvas before continuing.
 */                    
int bfree(void *ptr) {    
	//TODO: Your code goes in here.
	int abit_mask = 1;
	int pbit_mask = 2;
	int size_mask = 4088;

	//check is ptr is null
	if (ptr == NULL) {

		return -1;
	}
	//check for mutliple of 8
	if (((int)(ptr) % 8 != 0)) {
		return -1;
	}

	//check for ptr being outside of heap space
	if (ptr < (void*) heap_start || ptr >= (void*) heap_start + alloc_size){
		return -1;
	}

	//create the pointers blockheader
	blockHeader* currBlock = ptr-4;

	//make note of blocks true size
	int trueSize = (currBlock->size_status & size_mask);

	//check if block is already free
	if ((currBlock->size_status & abit_mask) == 0) {
		return -1;
	}

	//change the a bit on the curent block header to signify its free
	currBlock->size_status -= 1;

	//create the nextBlock header so we can change its p-bit
	blockHeader* nextBlock = (void*)(currBlock) + trueSize;

	//CHECK TO MAKE SURE NEXT BLOCK IS NOT END OF HEAP
	if (nextBlock->size_status != 1){
		if ((nextBlock->size_status & pbit_mask) == 2){
			nextBlock->size_status -= 2;
		}

	}
	//create a footer at the end of newly freed block
	blockHeader *footer = (void*) (currBlock) + trueSize - 4;
	footer->size_status = trueSize;
	//set the pointer to null signifying that we do not have valid memory anymore
	ptr = NULL;

	//run through the entire heap to check p bits on all blocks
	/* to do this, we will start at the beginning of the heap, and check whether a block is allocated or not
	   and check if their a and p bits match between current block and next block.
	   little bit of brute force to make p bits proper
	 */
	blockHeader *block = heap_start;
	while (block->size_status != 1){
		int pTrueSize = (block->size_status & size_mask);

		//check if current block is allocated or not (2 conditionals)

		//IF CURR NOT ALLOCATED, IS P BIT OF NEXT 0?
		if ((block->size_status & abit_mask) == 0) {
			blockHeader *pNext = (void*) (block) + pTrueSize;

			//check if the next block is not the heap end
			if (pNext->size_status != 1){
				//if not, then check to verify it's p bit
				if ((pNext->size_status & pbit_mask) != 0){
					//subtract 2 to remove the incorrect p-bit from nextBlock
					pNext->size_status-=2;
				}
			}
		}

		//IF CURR IS ALLOCATED, IS P BIT OF NEXT 2?
		if ((block->size_status & abit_mask) == 1) {
			blockHeader *pNext =(void*) (block) + pTrueSize;

			//check if the next block is not the heap end
			if (pNext->size_status != 1){
				//if not, then check to verify it's p bit
				if ((pNext->size_status & pbit_mask) != 2){
					//subtract 2 to remove the incorrect p-bit from nextBlock
					pNext->size_status += 2;
				}
			}
		}

		block = (void*) (block) +  pTrueSize;
	}

	// 1)case where next and previous block are NOT FREE (no coalescing)

	//check if previous block is allocated
	if ((currBlock->size_status & pbit_mask) == 2){
		//check if the next block is allocated (check a bit)
		if ((nextBlock->size_status & abit_mask) == 1 ) {
			//nothing needs to be done, return 0
			return 0;
		} 
	}

	// 2)case where next block is FREE and previous is NOT FREE 

	//check if previous block is allocated
	if ((currBlock->size_status & pbit_mask) == 2){
		//check if the next block is allcoated (a bit)
		if ((nextBlock->size_status & abit_mask) == 0) {

			//first, change size of current block to currentblock  size_status and
			//truesize of nextblock
			currBlock->size_status = currBlock->size_status + (nextBlock->size_status & size_mask);

			//update footer of new free block (add currBlock size_status -6 (2 for pbit and 4 for footer add)
			blockHeader *cFooter = (void*) (currBlock) + currBlock->size_status - 6;
			//set footer size
			cFooter->size_status = (currBlock->size_status & size_mask);
			//no need to change p bits or anything, finished with coalescing
			return 0;
		}

	}

	// 3) previous block IS FREE and next block IS NOT FREE

	//check if next block is allocated

	if ((nextBlock->size_status & abit_mask) == 1) {
		//check if previous block is free
		if ((currBlock->size_status & pbit_mask) == 0) {
			//then, we travel to the footer of prev block
			blockHeader *prevFooter =(void*) (currBlock) - 4;
			//get to previous header
			blockHeader *prevBlock = (void*)(currBlock) - prevFooter->size_status;

			//change prevBlock size_status to prevBlock->size_status + currBlock trueSize and add footer

			prevBlock->size_status = prevBlock->size_status + (currBlock->size_status & size_mask);

			//add footer to end of block using current prev block addr and true size (-4 due to location of footer)
			blockHeader *cFooter = (void*) (prevBlock) + (prevBlock->size_status & size_mask) - 4;
			//set footer size
			cFooter->size_status = (prevBlock->size_status & size_mask);
			return 0;
		}
	}

	// 4) previous and next block are BOTH FREE (coalesce both of them)

	//check if next block is free
	if ((nextBlock->size_status & abit_mask) == 0){
		//check if previous block is free
		if ((currBlock->size_status & pbit_mask) == 0) {
			//start with previous block coalesce
			blockHeader *prevFooter =(void*) (currBlock) - 4;
			//get to previous header
			blockHeader *prevBlock = (void*)(currBlock) - prevFooter->size_status;

			//change prevBlock size_status to prevBlock->size_status + currBlock trueSize and add footer

			prevBlock->size_status = prevBlock->size_status + (currBlock->size_status & size_mask);


			//then, use nextBlock size to combine what we made
			prevBlock->size_status = prevBlock->size_status + (nextBlock->size_status & size_mask);

			//set footer of super block
			blockHeader *cFooter = (void*) (prevBlock) + (prevBlock->size_status & size_mask) - 4;
			cFooter->size_status = (prevBlock->size_status & size_mask);
			return 0;	
		}
	} 
	return 0;
} 


/* 
 * Initializes the memory allocator.
 * Called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int init_heap(int sizeOfRegion) {    

	static int allocated_once = 0; //prevent multiple myInit calls

	int   pagesize; // page size
	int   padsize;  // size of padding when heap size not a multiple of page size
	void* mmap_ptr; // pointer to memory mapped area
	int   fd;

	blockHeader* end_mark;

	if (0 != allocated_once) {
		fprintf(stderr, 
				"Error:mem.c: InitHeap has allocated space during a previous call\n");
		return -1;
	}

	if (sizeOfRegion <= 0) {
		fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
		return -1;
	}

	// Get the pagesize from O.S. 
	pagesize = getpagesize();

	// Calculate padsize as the padding required to round up sizeOfRegion 
	// to a multiple of pagesize
	padsize = sizeOfRegion % pagesize;
	padsize = (pagesize - padsize) % pagesize;

	alloc_size = sizeOfRegion + padsize;

	// Using mmap to allocate memory
	fd = open("/dev/zero", O_RDWR);
	if (-1 == fd) {
		fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
		return -1;
	}
	mmap_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (MAP_FAILED == mmap_ptr) {
		fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
		allocated_once = 0;
		return -1;
	}

	allocated_once = 1;

	// for double word alignment and end mark
	alloc_size -= 8;

	// Initially there is only one big free block in the heap.
	// Skip first 4 bytes for double word alignment requirement.
	heap_start = (blockHeader*) mmap_ptr + 1;

	// Set the end mark
	end_mark = (blockHeader*)((void*)heap_start + alloc_size);
	end_mark->size_status = 1;

	// Set size in header
	heap_start->size_status = alloc_size;

	// Set p-bit as allocated in header
	// note a-bit left at 0 for free
	heap_start->size_status += 2;

	// Set the footer
	blockHeader *footer = (blockHeader*) ((void*)heap_start + alloc_size - 4);
	footer->size_status = alloc_size;

	return 0;
} 

/* STUDENTS MAY EDIT THIS FUNCTION, but do not change function header.
 * TIP: review this implementation to see one way to traverse through
 *      the blocks in the heap.
 *
 * Can be used for DEBUGGING to help you visualize your heap structure.
 * It traverses heap blocks and prints info about each block found.
 * 
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void disp_heap() {     

	int    counter;
	char   status[6];
	char   p_status[6];
	char * t_begin = NULL;
	char * t_end   = NULL;
	int    t_size;

	blockHeader *current = heap_start;
	counter = 1;

	int used_size =  0;
	int free_size =  0;
	int is_used   = -1;

	fprintf(stdout, 
			"********************************** HEAP: Block List ****************************\n");
	fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
	fprintf(stdout, 
			"--------------------------------------------------------------------------------\n");

	while (current->size_status != 1) {
		t_begin = (char*)current;
		t_size = current->size_status;

		if (t_size & 1) {
			// LSB = 1 => used block
			strcpy(status, "alloc");
			is_used = 1;
			t_size = t_size - 1;
		} else {
			strcpy(status, "FREE ");
			is_used = 0;
		}

		if (t_size & 2) {
			strcpy(p_status, "alloc");
			t_size = t_size - 2;
		} else {
			strcpy(p_status, "FREE ");
		}

		if (is_used) 
			used_size += t_size;
		else 
			free_size += t_size;

		t_end = t_begin + t_size - 1;

		fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status, 
				p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);

		current = (blockHeader*)((char*)current + t_size);
		counter = counter + 1;
	}

	fprintf(stdout, 
			"--------------------------------------------------------------------------------\n");
	fprintf(stdout, 
			"********************************************************************************\n");
	fprintf(stdout, "Total used size = %4d\n", used_size);
	fprintf(stdout, "Total free size = %4d\n", free_size);
	fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
	fprintf(stdout, 
			"********************************************************************************\n");
	fflush(stdout);

	return;  
} 


//		p3Heap.c (SP24)                     

