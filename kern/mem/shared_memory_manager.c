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
void sharing_init() {
#if USE_KHEAP
	LIST_INIT(&AllShares.shares_list)
	;
	init_spinlock(&AllShares.shareslock, "shares lock");
#else
	panic("not handled when KERN HEAP is disabled");
#endif
}

//==============================
// [2] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName) {
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
inline struct FrameInfo** create_frames_storage(int numOfFrames) {
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_frames_storage()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_frames_storage is not implemented yet");
	//Your Code is Here...
	struct FrameInfo** frames_storage = kmalloc(numOfFrames * sizeof(struct FrameInfo*));

	if (frames_storage == NULL) {
//        panic("No memory available for frames storage!");
		return NULL;
	}
	for (int i = 0; i < numOfFrames; i++) {
		frames_storage[i] = NULL;
	}
	return frames_storage;
}

//=====================================
// [2] Alloc & Initialize Share Object:
//=====================================
//Allocates a new shared object and initialize its member
//It dynamically creates the "framesStorage"
//Return: allocatedObject (pointer to struct Share) passed by reference
struct Share* create_share(int32 ownerID, char* shareName, uint32 size,
		uint8 isWritable) {
	//TODO: [PROJECT'24.MS2 - #16] [4] SHARED MEMORY - create_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("create_share is not implemented yet");
	//Your Code is Here...

	struct Share* shareObj = kmalloc(sizeof(struct Share));
	if (shareObj == NULL) {
//        panic("No memory available for creating Share object!");
		return NULL;

	}
	int numOfFrames = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	shareObj->ID = (int32) shareObj & ~0x80000000;
	shareObj->references = 1;
	strcpy(shareObj->name, shareName);
	shareObj->ownerID = ownerID;
	shareObj->size = size;
	shareObj->isWritable = isWritable;
	shareObj->framesStorage = create_frames_storage(numOfFrames);
	if (shareObj->framesStorage == NULL) {
		kfree(shareObj);
		return NULL;
	}
	return shareObj;
}

//=============================
// [3] Search for Share Object:
//=============================
//Search for the given shared object in the "shares_list"
//Return:
//	a) if found: ptr to Share object
//	b) else: NULL
struct Share* get_share(int32 ownerID, char* name) {
	//TODO: [PROJECT'24.MS2 - #17] [4] SHARED MEMORY - get_share()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("get_share is not implemented yet");
	//Your Code is Here...
  acquire_spinlock(&AllShares.shareslock);
	struct Share* share;
	LIST_FOREACH(share ,&AllShares.shares_list)
	{
		if (share->ownerID == ownerID && strcmp(share->name, name) == 0) {
			  release_spinlock(&AllShares.shareslock);
			return share;
		}
	}
	release_spinlock(&AllShares.shareslock);
	return NULL;
}

//=========================
// [4] Create Share Object:
//=========================
int createSharedObject(int32 ownerID, char* shareName, uint32 size,uint8 isWritable, void* virtual_address) {
	//TODO: [PROJECT'24.MS2 - #19] [4] SHARED MEMORY [KERNEL SIDE] - createSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("createSharedObject is not implemented yet");
	//Your Code is Here...

	//cprintf("createSharedObject: allocating size %d at %x \n", size,virtual_address);

	if (get_share(ownerID, shareName) != NULL) {
		return E_SHARED_MEM_EXISTS;
	}

	struct Share* new_share = create_share(ownerID, shareName, size,isWritable);
	if (new_share == NULL) {
		return E_NO_SHARE;
	}

    int numOfFrames = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;

	new_share->framesStorage = create_frames_storage(numOfFrames);
	if (new_share->framesStorage == NULL) {
		kfree(new_share);
		return E_NO_SHARE;
	}


	struct Env* myenv = get_cpu_proc();
	void* currVA = virtual_address;


		for (int i = 0; i < numOfFrames; i++) {
			uint32* pageTable;

			struct FrameInfo* frame;
			int result = allocate_frame(&frame);
			if (result != 0) {
				for (int j = 0; j < i; j++){
					struct FrameInfo* frame_to_free =get_frame_info(myenv->env_page_directory ,(uint32) currVA-(i-j)*PAGE_SIZE+1 ,&pageTable);
					free_frame(frame_to_free);
				}
				return E_NO_SHARE;
			}
			//cprintf("createSharedObject: frame size %d  \n", i);
	        result = map_frame(myenv->env_page_directory, frame, (uint32)currVA, PERM_WRITEABLE | PERM_USER | PERM_PRESENT);
			if (result != 0) {
				for (int j = 0; j < i; j++){
					uint32* pageTable;
					struct FrameInfo* frame_to_free =get_frame_info(myenv->env_page_directory ,(uint32) currVA-(i-j)*PAGE_SIZE+1 ,&pageTable);
					unmap_frame(myenv->env_page_directory , (uint32) currVA-(i-j)*PAGE_SIZE+1);
					free_frame(frame_to_free);
				}
				return E_NO_SHARE;
			}

			new_share->framesStorage[i] = frame;
			currVA += PAGE_SIZE;
		}
//	else
//		return E_NO_SHARE;
    acquire_spinlock(&AllShares.shareslock);
	LIST_INSERT_HEAD(&AllShares.shares_list, new_share);
	release_spinlock(&AllShares.shareslock);
	return (int) (new_share->ID);

}

//======================
// [5] Get Share Object:
//======================
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address) {
	//TODO: [PROJECT'24.MS2 - #21] [4] SHARED MEMORY [KERNEL SIDE] - getSharedObject()
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("getSharedObject is not implemented yet");
	//Your Code is Here..
    struct Share* ptr_share = get_share(ownerID, shareName);
    if (ptr_share == NULL) {
        return E_SHARED_MEM_NOT_EXISTS;
    }

    int numOfFrames = (ptr_share->size + PAGE_SIZE - 1) / PAGE_SIZE;

    struct Env* myenv = get_cpu_proc();
    if (myenv == NULL) {
        return E_SHARED_MEM_NOT_EXISTS;
    }

    void* currVA = virtual_address;

    for (int i = 0; i < numOfFrames; i++) {
        struct FrameInfo* frame = ptr_share->framesStorage[i];
        if (frame == NULL) {
            return E_NO_SHARE;
        }

        int perm = PERM_USER | PERM_PRESENT;
        if (ptr_share->isWritable) {
            perm |= PERM_WRITEABLE;
        }

        int result = map_frame(myenv->env_page_directory, frame, (uint32)currVA, perm);
		if (result != 0) {
			return E_NO_SHARE;
		}

        currVA += PAGE_SIZE;
    }


    ptr_share->references++;
    return (int)(ptr_share->ID);
}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//==========================
// [B1] Delete Share Object:
//==========================
//delete the given shared object from the "shares_list"
//it should free its framesStorage and the share object itself
void free_share(struct Share* ptrShare) {
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - free_share()

    if (ptrShare == NULL) {
        cprintf("free_share: Invalid shared object\n");
        return;
    }

    acquire_spinlock(&AllShares.shareslock);
    LIST_REMOVE(&AllShares.shares_list, ptrShare); // Remove from list
    release_spinlock(&AllShares.shareslock);

    // Free frames storage
    if (ptrShare->framesStorage != NULL) {
        kfree(ptrShare->framesStorage);
    }

    // Free the shared object itself
    kfree(ptrShare);

    cprintf("free_share: Successfully deleted shared object (OwnerID: %d, Name: %s)\n",ptrShare->ownerID, ptrShare->name);

}

//========================
// [B2] Free Share Object:
//========================
struct Share* sharedPages[NUM_OF_UHEAP_PAGES];

struct Share* getShareObjectByVA(void* virtual_address) {
	//TODO: [PROJECT'24.MS2 - #17] [++] SHARED MEMORY - get_share_by_va()

	uint32 share_va_index = ((uint32)virtual_address - USER_HEAP_START) / PAGE_SIZE;
	return sharedPages[share_va_index];
}

int freeSharedObject(int32 sharedObjectID, void *startVA) {
	//TODO: [PROJECT'24.MS2 - BONUS#4] [4] SHARED MEMORY [KERNEL SIDE] - freeSharedObject()

    struct Share* ptr_share = (struct Share*)getShareObjectByVA(startVA);
    if (ptr_share == NULL) {
        return E_SHARED_MEM_NOT_EXISTS;
    }

    struct Env *e = get_cpu_proc(); // Get current environment
    if (e == NULL) {
        return E_SHARED_MEM_NOT_EXISTS; // Error: Environment not found
    }

    int numOfFrames = ROUNDUP(ptr_share->size, PAGE_SIZE) / PAGE_SIZE;

    void* current_address = startVA;

    uint32* page_table;


    for (int i = 0; i < numOfFrames; i++) {
        struct FrameInfo* frame = ptr_share->framesStorage[i];
        if (frame != NULL) {
			unmap_frame(e->env_page_directory, (uint32)current_address);
        }

        current_address += PAGE_SIZE;
    }
    current_address =startVA;

//    for (int i = 0; i < numOfFrames; i++) {
//        	if (get_page_table(e->env_page_directory, (uint32)current_address, &page_table) == TABLE_IN_MEMORY) {
//    				unmap_frame(e->env_page_directory, (uint32)page_table);
//
//        }
//    }

    // Reduce reference count
    ptr_share->references--;
    if (ptr_share->references == 0) {
        free_share(ptr_share); // Delete the shared object entirely
    }

    return 0;
}
