#include <inc/lib.h>
uint32 marked_pages[NUM_OF_UHEAP_PAGES]={0};

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{
	//cprintf("Size :%d\n",size);
//==============================================================
//DON'T CHANGE THIS CODE========================================
if (size == 0) return NULL ;
//==============================================================
//TODO: [PROJECT'24.MS2 - #12] [3] USER HEAP [USER SIDE] - malloc()
// Write your code here, remove the panic and write your code
//panic("malloc() is not implemented yet...!!");

/* if size <= 2KB => call the dynAllocator function*/
if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
{
	return alloc_block_FF((uint32)size);
}else if (size > ((uint32)USER_HEAP_MAX - (uint32)(myEnv->Limit + PAGE_SIZE)))
{
	return NULL;
}

size = ROUNDUP(size, PAGE_SIZE);
uint32 init_mark_add = 0;
uint32 count = 0;
uint32 freeSize = 0;
uint32 flag = 0;

for(uint32 curAddr = (uint32)myEnv->Limit + PAGE_SIZE; curAddr <= USER_HEAP_MAX - PAGE_SIZE; curAddr += PAGE_SIZE)
{
	if(freeSize >= size)
	{
		marked_pages[(init_mark_add-USER_HEAP_START)/PAGE_SIZE]=freeSize/PAGE_SIZE;
		sys_allocate_user_mem(init_mark_add, freeSize);
		flag = 1;
		break;
	}
	if(marked_pages[(curAddr-USER_HEAP_START)/PAGE_SIZE] > 0){
		curAddr+=(marked_pages[(curAddr-USER_HEAP_START)/PAGE_SIZE]*PAGE_SIZE)-PAGE_SIZE;
		count = 0;
		freeSize = 0;
		init_mark_add = 0;
	}else{
		if(!count) init_mark_add = curAddr;
		freeSize+=PAGE_SIZE;
		count++;
	}

}
if(flag){
	return (void*)init_mark_add;
}

return NULL;
//Use sys_isUHeapPlacementStrategyFIRSTFIT() andsys_isUHeapPlacementStrategyBESTFIT()
//to check the current strategy
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #14] [3] USER HEAP [USER SIDE] - free()
	// Write your code here, remove the panic and write your code
	//panic("free() is not implemented yet...!!");

	uint32 va = (uint32) virtual_address;

	// Block Allocator Range
	if (va >= USER_HEAP_START && va <= (uint32)(myEnv->Limit))
	{
		free_block(virtual_address);
	}
	else if (va >= ((uint32)(myEnv->Limit) + PAGE_SIZE) && va <= (USER_HEAP_MAX-PAGE_SIZE))
	{
			uint32 size = marked_pages[(va-USER_HEAP_START)/PAGE_SIZE]*PAGE_SIZE;
	        uint32 numOfPages = marked_pages[(va-USER_HEAP_START)/PAGE_SIZE];

	        for (uint32 curAddr = va; numOfPages != 0; curAddr+=PAGE_SIZE) {
				marked_pages[(curAddr-USER_HEAP_START)/PAGE_SIZE]=0;
				numOfPages--;
			}
	        sys_free_user_mem(va, size);
	 }
	 else
	 {
	 	 panic("Invalid Address");
	 }

}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #18] [4] SHARED MEMORY [USER SIDE] - smalloc()
	// Write your code here, remove the panic and write your code
	//panic("smalloc() is not implemented yet...!!");
	if (size > ((uint32)USER_HEAP_MAX - (uint32)(myEnv->Limit + PAGE_SIZE)))
	{
		return NULL;
	}
	size = ROUNDUP(size, PAGE_SIZE);
	uint32 init_mark_add = 0;
	uint32 count = 0;
	uint32 freeSize = 0;
	uint32 flag = 0;

	for(uint32 curAddr = (uint32)myEnv->Limit + PAGE_SIZE; curAddr <= USER_HEAP_MAX - PAGE_SIZE; curAddr += PAGE_SIZE)
	{
		if(freeSize >= size)
		{
			marked_pages[(init_mark_add-USER_HEAP_START)/PAGE_SIZE]=freeSize/PAGE_SIZE;
			flag = sys_createSharedObject(sharedVarName,size,isWritable,(void*)init_mark_add);
			break;
		}
		if(marked_pages[(curAddr-USER_HEAP_START)/PAGE_SIZE] > 0){

			curAddr+=(marked_pages[(curAddr-USER_HEAP_START)/PAGE_SIZE]*PAGE_SIZE)-PAGE_SIZE;
			count = 0;
			freeSize = 0;
			init_mark_add = 0;
		}else{

			if(!count) init_mark_add = curAddr;
			freeSize+=PAGE_SIZE;
			count++;
		}

	}
	if(flag!=E_NO_SHARE && flag!=E_SHARED_MEM_EXISTS){


		return (void*)init_mark_add;
	}

	return NULL;

}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//TODO: [PROJECT'24.MS2 - #20] [4] SHARED MEMORY [USER SIDE] - sget()
	// Write your code here, remove the panic and write your code
	//panic("sget() is not implemented yet...!!");


	if(sys_getSizeOfSharedObject(ownerEnvID, sharedVarName)== E_SHARED_MEM_NOT_EXISTS){
		return NULL;
	}
	uint32 size = ROUNDUP(sys_getSizeOfSharedObject(ownerEnvID, sharedVarName), PAGE_SIZE);
	uint32 init_mark_add = 0;
	uint32 count = 0;
	uint32 freeSize = 0;
	uint32 flag = 0;

	for(uint32 curAddr = (uint32)myEnv->Limit + PAGE_SIZE; curAddr <= USER_HEAP_MAX - PAGE_SIZE; curAddr += PAGE_SIZE)
	{
		if(freeSize >= size)
		{
			marked_pages[(init_mark_add-USER_HEAP_START)/PAGE_SIZE]=freeSize/PAGE_SIZE;
			flag = sys_getSharedObject(ownerEnvID,sharedVarName,(void*)init_mark_add);
			break;
		}
		if(marked_pages[(curAddr-USER_HEAP_START)/PAGE_SIZE] > 0){

			curAddr+=(marked_pages[(curAddr-USER_HEAP_START)/PAGE_SIZE]*PAGE_SIZE)-PAGE_SIZE;
			count = 0;
			freeSize = 0;
			init_mark_add = 0;
		}else{

			if(!count) init_mark_add = curAddr;
			freeSize+=PAGE_SIZE;
			count++;
		}

	}
	if( flag!= E_SHARED_MEM_NOT_EXISTS){


		return (void*)init_mark_add;
	}

	return NULL;

}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [USER SIDE] - sfree()
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//[PROJECT]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}
