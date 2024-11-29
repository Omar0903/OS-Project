#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/queue.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
struct Share* get_share(int32 ownerID, char* name);

//===========================
// [1] INITIALIZE SHARES:
//===========================
//Initialize the list and the corresponding lock
void sharing_init()
{
#if USE_KHEAP
	LIST_INIT(&AllShares.shares_list) ;
	init_spinlock(&AllShares.shareslock, "shares lock");
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

//==============================
// [2] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	//[PROJECT'24.MS2] DONE
	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//
	struct Share* ptr_share = get_share(ownerID, shareName);
	if (ptr_share == NULL)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return ptr_share->size;

	return 0;
}

//===========================================================


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
//===========================
// [1] Create frames_storage:
//===========================
// Create the frames_storage and initialize it by 0
inline struct FrameInfo** create_frames_storage(int numOfFrames)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_frames_storage()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_frames_storage is not implemented yet");
	//Your Code is Here...

    // Allocate memory for the array of pointers to FrameInfo
	uint32 va = (uint32)kmalloc(numOfFrames * sizeof(struct FrameInfo*));
    struct FrameInfo** framesStorage = (struct FrameInfo**) va;
    if (framesStorage == NULL) {
        // Allocation failed
        return NULL;
    }

    // Initialize the allocated memory to zero
    for (uint32 i = 0; i < numOfFrames; ++i) {
    	framesStorage[i] = NULL;
	}

    // Return the pointer to the allocated array
    return framesStorage;


}

//=====================================
// [2] Alloc & Initialize Share Object:
//=====================================
//Allocates a new shared object and initialize its member
//It dynamically creates the "framesStorage"
//Return: allocatedObject (pointer to struct Share) passed by reference
struct Share* create_share(int32 ownerID, char* shareName, uint32 size, uint8 isWritable)
{
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_share is not implemented yet");
	//Your Code is Here...

	struct Share *shared_obj = (struct Share*) kmalloc(sizeof(struct Share));
	if (shared_obj == NULL) {
		return NULL;
	}
	uint32 numFrames = ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;
	shared_obj->framesStorage = create_frames_storage(numFrames);
	if (shared_obj->framesStorage == NULL) {
		kfree(shared_obj);
		return NULL;
	}
	shared_obj->ID = (uint32) shared_obj & 0x7fffffff;
	shared_obj->ownerID= ownerID;
//	strncpy(shared_obj->name, shareName, sizeof(shared_obj->name) - 1);
//	shared_obj->name[sizeof(shared_obj->name) - 1] = '\0'; // Ensure null termination
	strncpy(shared_obj->name, shareName, sizeof(shared_obj->name));
	shared_obj->size=size;
	shared_obj->isWritable=isWritable;
	shared_obj->references=1;

	return shared_obj;

}

//=============================
// [3] Search for Share Object:
//=============================
//Search for the given shared object in the "shares_list"
//Return:
//	a) if found: ptr to Share object
//	b) else: NULL
struct Share* get_share(int32 ownerID, char* name)
{
	//TODO: [PROJECT'24.MS2 - #17] [4] SHARED MEMORY - get_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_share is not implemented yet");
	//Your Code is Here...
	struct Share* share_obj;

	LIST_FOREACH(share_obj,&AllShares.shares_list){
		int iscompared = strcmp(share_obj->name,name)==0 ;
		if(share_obj->ownerID==ownerID && iscompared)
		{
			return share_obj;
		}
	}
	return NULL;

}

//=========================
// [4] Create Share Object:
//=========================
int createSharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #19] [4] SHARED MEMORY [KERNEL SIDE] - createSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("createSharedObject is not implemented yet");
	//Your Code is Here...
	struct Env* myenv = get_cpu_proc(); //The calling environment

	struct Share *existShare;
//	acquire_spinlock(&AllShares.shareslock);
	LIST_FOREACH(existShare,&AllShares.shares_list){
		if (existShare->ownerID == ownerID && strcmp(existShare->name,shareName)==0) {
			return E_SHARED_MEM_EXISTS;
		}
	}
//	release_spinlock(&AllShares.shareslock);
//	if(getSizeOfSharedObject(ownerID, shareName)!= E_SHARED_MEM_NOT_EXISTS){
//		return E_SHARED_MEM_EXISTS;
//	}

	struct Share *newShare = create_share(ownerID,shareName,size,isWritable);
	if (newShare == NULL) {
		return E_NO_SHARE;
	}
	acquire_spinlock(&AllShares.shareslock);
	LIST_INSERT_HEAD(&AllShares.shares_list,newShare);
	release_spinlock(&AllShares.shareslock);

//	uint32 count = 0;
//	for (uint32 i = (uint32)virtual_address;  i <= ((uint32)virtual_address)+size ; i+= PAGE_SIZE) {
//		struct FrameInfo* frame = NULL;
//		allocate_frame(&frame);
//		map_frame(myenv->env_page_directory,frame,i,PERM_USER|PERM_PRESENT|PERM_WRITEABLE);
//		newShare->framesStorage[count]=frame;
//		count++;
//
//	}
	uint32 numofFrames = ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;
	uint32 va = (uint32) virtual_address;
	for (uint32 i  = 0;  i  < numofFrames; i++ ) {
		struct FrameInfo* frame = NULL;
		allocate_frame(&frame);
		map_frame(myenv->env_page_directory,frame,va,PERM_USER|PERM_PRESENT|PERM_WRITEABLE);
		newShare->framesStorage[i]=frame;
		va+=PAGE_SIZE;

	}

	return newShare->ID;

}


//======================
// [5] Get Share Object:
//======================
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("getSharedObject is not implemented yet");
	//Your Code is Here...
	struct Env* myenv = get_cpu_proc(); //The calling environment
	struct Share* shared_obj = NULL;
	struct Share* found_obj = NULL ;
	LIST_FOREACH(shared_obj,&AllShares.shares_list){
		int iscompared = strcmp(shared_obj->name,shareName)==0 ;
		if(shared_obj->ownerID==ownerID &&iscompared){
			found_obj = shared_obj;
			break;
		}
	}
	if (found_obj == NULL){
		return E_SHARED_MEM_NOT_EXISTS;
	}
	uint32 numofFrames = ROUNDUP(found_obj->size,PAGE_SIZE)/PAGE_SIZE;
	uint32 va = (uint32) virtual_address;
	for (uint32 i = 0; i < numofFrames; i++) {
		struct FrameInfo* frame = found_obj->framesStorage[i];
		int perm;
		if(found_obj->isWritable){
			perm = PERM_WRITEABLE | PERM_PRESENT | PERM_USER;
		}else{
			perm = PERM_PRESENT | PERM_USER;
		}
		map_frame(myenv->env_page_directory,frame,va,perm);
		va+= PAGE_SIZE;

	}
	found_obj->references++;
	return (found_obj->ID);

}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//==========================
// [B1] Delete Share Object:
//==========================
//delete the given shared object from the "shares_list"
//it should free its framesStorage and the share object itself
void free_share(struct Share* ptrShare)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - free_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("free_share is not implemented yet");
	//Your Code is Here...

}
//========================
// [B2] Free Share Object:
//========================
int freeSharedObject(int32 sharedObjectID, void *startVA)
{
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - freeSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	panic("freeSharedObject is not implemented yet");
	//Your Code is Here...

}
