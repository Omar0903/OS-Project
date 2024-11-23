#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

#define Mega  (1024*1024)
#define kilo (1024)


int index = 0;

struct AllocationInfo
{
	uint32 *va;
	uint32 size;
}allocationStorage[999999];



//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'24.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator
	// Write your code here, remove the panic and write your code
	initSizeToAllocate = ROUNDUP(initSizeToAllocate, PAGE_SIZE);
	Start = (uint32*)daStart;
	MappedRegion = (uint32*)(daStart+initSizeToAllocate);
	Limit = (uint32*)daLimit;

	for(uint32 i = (uint32)Start; i<(uint32)MappedRegion; i += PAGE_SIZE)
	{
		struct FrameInfo *ptr_frame_info=NULL;
		allocate_frame(&ptr_frame_info);
		ptr_frame_info->virtaddr = i;
		map_frame(ptr_page_directory, ptr_frame_info, (uint32)i, PERM_WRITEABLE|PERM_PRESENT);
	}

	initialize_dynamic_allocator(daStart, initSizeToAllocate);
	return 0;
}

void* sbrk(int numOfPages)
{
	//cprintf("hi from sbrk \n");
	/* numOfPages > 0: move the segment break of the kernel to increase the size of its heap by the given numOfPages,
	 * 				you should allocate pages and map them into the kernel virtual address space,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * numOfPages = 0: just return the current position of the segment break
	 *
	 * NOTES:
	 * 	1) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, return -1
	 */

	//MS2: COMMENT THIS LINE BEFORE START CODING==========
	//return (void*)-1 ;
	//====================================================

	//TODO: [PROJECT'24.MS2 - #02] [1] KERNEL HEAP - sbrk
	// Write your code here, remove the panic and write your code
	uint32 prevbreak = (uint32)MappedRegion;
	//cprintf("PREV BREAK:%x\n",prevbreak);
	if(numOfPages == 0)
	{
		//cprintf("[[ num of pages = 0 ]] \n");
		return (void*)prevbreak;
	}

	if((uint32)prevbreak + (numOfPages*PAGE_SIZE) > (uint32)Limit)
	{
		//cprintf("got in here... \n");
		return (void*)-1;
	}
	else
	{
		//cprintf("got in THERE.... \n");
		MappedRegion += (numOfPages*kilo);
		//cprintf("MappedRegion:%x\n",MappedRegion);
		for(uint32 i = (uint32)prevbreak; i <= (uint32)MappedRegion - PAGE_SIZE; i += PAGE_SIZE)
		{
			struct FrameInfo *ptr_frame_info=NULL;
			allocate_frame(&ptr_frame_info);
			ptr_frame_info->virtaddr = i;
			map_frame(ptr_page_directory,ptr_frame_info, i, PERM_WRITEABLE|PERM_PRESENT);
		}
		//cprintf("prevbreak: %x\n", prevbreak);
		return (void*)prevbreak;
	}
}

//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator

void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'24.MS2 - #03] [1] KERNEL HEAP - kmalloc
	// Write your code here, remove the panic and write your code

	//cprintf("Required Size: %d\n", size);
	/* if size <= 2KB => call the blockAllocator function*/
	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{
		return alloc_block_FF((uint32)size);
	}
	else if (size > ((uint32)KERNEL_HEAP_MAX - (uint32)(Limit + PAGE_SIZE)))
	{
		return NULL;
	}

	size = ROUNDUP(size, PAGE_SIZE);

	uint32 init_alloc_add = 0;
	uint32 *ptr_page_table = NULL;
	uint32 freeSize = 0;
	uint32 count = 0;
	bool flag = 0;

	//cprintf("init_alloc_add before the LOOP: %x\n", init_alloc_add);

	for(uint32 cur_add = (uint32)Limit + PAGE_SIZE; (uint32)cur_add <= KERNEL_HEAP_MAX - PAGE_SIZE; cur_add += PAGE_SIZE)
	{
		//cprintf("Current Address: %x\n", cur_add);
	    /* Check if free size is sufficient */
	    if (freeSize >= size)
	    {
	        for(uint32 i = init_alloc_add; i <= cur_add - PAGE_SIZE; i += PAGE_SIZE)
	        {
	            struct FrameInfo *ptr_frame = NULL;
	            allocate_frame(&ptr_frame);
	            ptr_frame->virtaddr = i;
	            map_frame(ptr_page_directory, ptr_frame, (uint32)i, PERM_WRITEABLE|PERM_PRESENT);
	        }
	        //cprintf("init_alloc_add: %x\n", init_alloc_add);
	        flag = 1;
	        break;
	    }

	    /*Get page table of the VA and check the present bit of the table entry*/
	    get_page_table(ptr_page_directory, (uint32)cur_add, &ptr_page_table);
	    uint32 tableEntry = ptr_page_table[PTX(cur_add)];
	    uint32 presentBit = tableEntry & 1;
	    if(!presentBit)
	    {
	    	if(!count) init_alloc_add = cur_add;
	    	freeSize += PAGE_SIZE;
	    	count++;
	    }
	    else
	    {
	    	 freeSize = 0;
	    	 count = 0;
	    	 init_alloc_add = 0;
	    }
	}

	//cprintf("=============== Returned Address: =============== %x\n", (void*)init_alloc_add);

	allocationStorage[index].va = (uint32*)init_alloc_add;
	allocationStorage[index].size = size;
	index++;

	if(flag) return (void*)init_alloc_add;
	return NULL;
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
}

void kfree(void* virtual_address)
{
	// Block Allocator Range
    if ((uint32)virtual_address >= (uint32)KERNEL_HEAP_START && (uint32)virtual_address <= (uint32)Limit) // Block Allocator Range
    {
        free_block((void*)virtual_address);
    }
    // Page Allocator Range
    else if ((uint32)virtual_address >= ((uint32)Limit + PAGE_SIZE) && (uint32)virtual_address <= (uint32)KERNEL_HEAP_MAX) // Page Allocator Range
    {
        uint32 numOfPages = 0; // Number of pages to free
        uint32 delIndex = 0; // Delete this index from array after freeing

        // Get number of pages for allocation and index to remove
        for (uint32 i = 0; i < index; i++)
        {
            if (virtual_address != allocationStorage[i].va)
            {
                continue;
            }
            else
            {
                numOfPages = (ROUNDUP(allocationStorage[i].size, PAGE_SIZE)) / PAGE_SIZE;
                delIndex = i;
                break;
            }
        }

        // Round down address (Example: ROUNDDOWN(0x4102,0x1000(4KB)) = 0x4000 -Point to the beginning of the page)
        uint32* alignedAddress = (uint32*)ROUNDDOWN((uint32)virtual_address, PAGE_SIZE);
        uint32* pageTable;
        struct FrameInfo* frameInfo;

        for (uint32 i = 0; i < numOfPages; i++)
        {
            frameInfo = get_frame_info(ptr_page_directory, (uint32)alignedAddress, &pageTable);

            if (frameInfo != NULL)
            {
                unmap_frame(ptr_page_directory, (uint32)alignedAddress);
            }

            alignedAddress += (PAGE_SIZE / sizeof(uint32)); // Move to the next page
        }

        //Remove allocation from the array
        for (uint32 j = delIndex; j < index - 1; j++)
        {
            allocationStorage[j] = allocationStorage[j + 1];
        }

        index--;

    }
    // Address out of range
    else
    {
        panic("Invalid Address");
    }
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");

	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
	uint32* ptr_page_table = NULL;
	get_page_table(ptr_page_directory,virtual_address,&ptr_page_table);

	struct FrameInfo *frame = get_frame_info(ptr_page_directory, virtual_address, &ptr_page_table);
	if(frame == NULL)
	{
		return 0;
	}

	uint32 pa = (ptr_page_table[PTX(virtual_address)] & KERNEL_HEAP_MAX)+(virtual_address & 0x00000FFF);

	return pa;
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
	struct FrameInfo* target_frame = to_frame_info(physical_address);
	uint32 expectedVA = ((target_frame->virtaddr & KERNEL_HEAP_MAX) + (physical_address & 0x00000FFF));
	if(target_frame->virtaddr == 0){
		return 0;
	}
	return expectedVA;
}
//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, if moved to another loc: the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
	// Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}
