/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"



//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
__inline__ uint32 get_block_size(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (*curBlkMetaData) & ~(0x1);
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
__inline__ int8 is_free_block(void* va)
{
	uint32 *curBlkMetaData = ((uint32 *)va - 1) ;
	return (~(*curBlkMetaData) & 0x1) ;
}

//===========================
// 3) ALLOCATE BLOCK:
//===========================

void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockElement* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", get_block_size(blk), is_free_block(blk)) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (initSizeOfAllocatedSpace % 2 != 0) initSizeOfAllocatedSpace++; //ensure it's multiple of 2
		if (initSizeOfAllocatedSpace == 0)
			return ;
		is_initialized = 1;
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #04] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("initialize_dynamic_allocator is not implemented yet");
	//Your Code is Here...
	//initialize_heap(daStart,daStart+initSizeOfAllocatedSpace);
	uint32 *beg_block=(uint32*)(daStart);
	uint32 *end_block=(uint32*)(daStart+initSizeOfAllocatedSpace - sizeof(int));
	*beg_block=0x00000001;
	*end_block=0x00000001;
	uint32 *header=(uint32*)(daStart+sizeof(int));
	uint32 *footer=(uint32*)(daStart+initSizeOfAllocatedSpace -2*sizeof(int));
	*header=initSizeOfAllocatedSpace - 2*sizeof(int);
	*footer=initSizeOfAllocatedSpace - 2*sizeof(int);
	struct BlockElement *block=(struct BlockElement*)(header+1);
	LIST_INIT(&freeBlocksList);
	LIST_INSERT_HEAD(&freeBlocksList,block);


}
//==================================
// [2] SET BLOCK HEADER & FOOTER:
//==================================
void set_block_data(void* va, uint32 totalSize, bool isAllocated)
{
	//TODO: [PROJECT'24.MS1 - #05] [3] DYNAMIC ALLOCATOR - set_block_data
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("set_block_data is not implemented yet");
	//Your Code is Here...
	uint32 *header = (uint32*)((uint32)va - sizeof(uint32));
	uint32 *footer = (uint32*)((uint32)va + totalSize - 2*sizeof(uint32));
	*header = *footer = totalSize;

	if(isAllocated)
	{
		*header = *header | 0x00000001;
		*footer = *footer | 0x1;
	}
	else
	{
		*header = *header & 0xFFFFFFFE ;
		*footer = *footer & 0xFFFFFFFE ;
	}
}
//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (size % 2 != 0) size++;	//ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE ;
		if (!is_initialized)
		{
			uint32 required_size = size + 2*sizeof(int) /*header & footer*/ + 2*sizeof(int) /*da begin & end*/ ;
			uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE)/PAGE_SIZE);
			uint32 da_break = (uint32)sbrk(0);
			initialize_dynamic_allocator(da_start, da_break - da_start);
		}
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #06] [3] DYNAMIC ALLOCATOR - alloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("alloc_block_FF is not implemented yet");
	//Your Code is Here...
	struct BlockElement *block; /*pointer to the block element*/
	uint32 metaData = 2*sizeof(uint32); /* 8 bytes => (header & footer) */
	bool flag = 0; /*flag to if the free block found or not*/

	/*Looping on free Blocks in the freeBlock list*/
	LIST_FOREACH(block, &freeBlocksList){
		uint32 blockSize = get_block_size(block); /*size of the freeblock*/
		if(blockSize >= size + metaData)     /* if the required size found */
		{
			if(blockSize - (size+metaData) >= 4*sizeof(uint32) /*16 byte*/) /*if the remaining size is large*/
			{
	//splitting it ...
				struct BlockElement *freeBlock = (struct BlockElement *)( (uint32)block + size + 2*sizeof(uint32)); /*residual free block*/
				set_block_data((void*) block, (size+metaData), 1);
				set_block_data((void*)freeBlock, blockSize - (size+metaData), 0);
				LIST_INSERT_AFTER(&freeBlocksList, block, freeBlock);
				flag = 1;
			}
			else if(blockSize - (size+metaData) < 4*sizeof(uint32) /*16 byte*/)
			{
				cprintf("2\n");
				//Ignoring the remaining size (internal fragmentation) ...
				set_block_data((void*) block, blockSize, 1);
			}
			LIST_REMOVE(&freeBlocksList, block);
			flag = 1;
			break;
		}
	}
	if(!flag)
	{
		sbrk(size);
		return NULL;
	}
	return (void*) block;
}

//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void* alloc_block_BF(uint32 size) {
    if (size % 2 != 0) size++; // Ensure size is even
    if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
        size = DYN_ALLOC_MIN_BLOCK_SIZE;

    if (!is_initialized) {
        uint32 required_size = size + 2 * sizeof(int) /* header & footer */ + 2 * sizeof(int) /* da begin & end */;
        uint32 da_start = (uint32)sbrk(ROUNDUP(required_size, PAGE_SIZE) / PAGE_SIZE);
        uint32 da_break = (uint32)sbrk(0);
        initialize_dynamic_allocator(da_start, da_break - da_start);
    }

    struct BlockElement *block = NULL; // Pointer to the current block
    struct BlockElement *best_fit = NULL; // Pointer to the best-fit block found
    uint32 best_fit_size = 4294967295; // Track the size of the best-fit block

    // Loop through free blocks in the freeBlock list
    LIST_FOREACH(block, &freeBlocksList) {
        uint32 blockSize = get_block_size(block); // Size of the free block
        if (blockSize >= size + (2 * sizeof(uint32))) { // Check if the block is large enough
            if (blockSize < best_fit_size) { // Find the smallest block that fits
                best_fit = block;
                best_fit_size = blockSize;
            }
        }
    }

    // If a suitable block was found
    if (best_fit) {
        uint32 blockSize = best_fit_size;
        // Check if the block can be split
        if (blockSize - (size + (2 * sizeof(uint32))) >= 4 * sizeof(uint32)) { // If remaining space is large enough
            // Splitting the block
            struct BlockElement *freeBlock = (struct BlockElement *)((uint32)best_fit + size + (2 * sizeof(uint32)));
            set_block_data((void*)best_fit, (size + (2 * sizeof(uint32))), 1); // Set allocated block data
            set_block_data((void*)freeBlock, blockSize - (size + (2 * sizeof(uint32))), 0); // Set new free block data
            LIST_INSERT_AFTER(&freeBlocksList, best_fit, freeBlock); // Insert new free block into the list
        } else {
            // Use the whole block
            set_block_data((void*)best_fit, blockSize, 1);
        }
        LIST_REMOVE(&freeBlocksList, best_fit); // Remove the allocated block from the free list
        return (void*)best_fit; // Return the allocated block
    }

    // If no suitable block was found, expand the heap
    void *new_block = sbrk(size + (2 * sizeof(uint32)));
    if (new_block == (void *)-1) {
        return NULL; // Allocation failed
    }
    set_block_data(new_block, size + (2 * sizeof(uint32)), 1); // Initialize the new block
    return new_block; // Return the new block
}



//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'24.MS1 - #07] [3] DYNAMIC ALLOCATOR - free_block
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("free_block is not implemented yet");
	//Your Code is Here...

	if (va == NULL)
	{
		return;
	}
	else
	{   uint32 original_block_size=get_block_size(va);
		uint32 block_size = get_block_size(va);
		struct BlockElement *block= va;
		struct BlockElement *prevFreeBlock = NULL;
		struct BlockElement *nextFreeBlock = NULL;

		struct BlockElement *element;

		LIST_FOREACH(element, &freeBlocksList)
		{
			if (element < block)
			{
				prevFreeBlock = element;
			}
			else if (element > block)
			{
				nextFreeBlock = element;
				break;

			}
		}

		uint32 prevBlocksize=get_block_size(prevFreeBlock);

		if (prevFreeBlock != NULL && ((void*)((uint8*)prevFreeBlock)+ prevBlocksize)  == va)
		{
			block_size += get_block_size(prevFreeBlock);
			block = prevFreeBlock;
			LIST_REMOVE(&freeBlocksList, prevFreeBlock);

		}

		uint32 nextBlocksize=get_block_size(nextFreeBlock);

		if (nextFreeBlock != NULL && ((void*)((uint8*)nextFreeBlock - original_block_size)) == va)
		{
			block_size += get_block_size(nextFreeBlock);
			LIST_REMOVE(&freeBlocksList, nextFreeBlock);
		}

		set_block_data((void*)block, block_size, 0);


		struct BlockElement *pprevFreeBlock = NULL;
		struct BlockElement *nnextFreeBlock = NULL;

		LIST_FOREACH(element, &freeBlocksList)
		{
			if (element < block)
			{
				pprevFreeBlock = element;
			}
			else if (element > block)
			{
				nnextFreeBlock = element;
				break;

			}
		}


		if (pprevFreeBlock == NULL)
		{
			LIST_INSERT_HEAD(&freeBlocksList, block);
		}
		else if (nnextFreeBlock == NULL)
		{
			LIST_INSERT_TAIL(&freeBlocksList, block);
		}
		else
		{
			LIST_INSERT_AFTER(&freeBlocksList, pprevFreeBlock, block);
		}
	}
}
//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'24.MS1 - #08] [3] DYNAMIC ALLOCATOR - realloc_block_FF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("realloc_block_FF is not implemented yet");
	//Your Code is Here...
}

/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/
//=========================================
// [7] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [8] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}
