#include <inc/lib.h>
//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment) {
//	cprintf("userSbrk: calling sys_sbrk \n");
	void* brk = sys_sbrk(increment);
//	cprintf("userSbrk: sys_sbrk returned %x \n" , brk);

	return brk;
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================

uint32 reservedPages[NUM_OF_UHEAP_PAGES] = { 0 };

void* malloc(uint32 size) {
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0)
		return NULL;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #12] [3] USER HEAP [USER SIDE] - malloc()
	// Write your code here, remove the panic and write your code

	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy

	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE
			&& sys_isUHeapPlacementStrategyFIRSTFIT()) {
		void *block = alloc_block_FF(size);

//		cprintf("malloc: call alloc_FF returned %x \n" ,(uint32*)block);

		return block;

	}

//	cprintf("malloc: NUM_OF_UHEAP_PAGES %d \n",NUM_OF_UHEAP_PAGES);

//	cprintf("malloc: page_allocator_start index %d \n",page_allocator_start/PAGE_SIZE);

	uint32 page_allocator_start = myEnv->heapHardLimit + PAGE_SIZE;
	uint32 page_allocator_end = USER_HEAP_MAX;
	uint32 num_of_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	uint32 start_of_allocation;
	uint32 pages_count = 0;
	for (uint32 i = (page_allocator_start - USER_HEAP_START) / PAGE_SIZE;
			i < NUM_OF_UHEAP_PAGES; i++) {
//		cprintf("malloc: pages_count %d \n",pages_count);
		if (reservedPages[i] == 0)
			pages_count++;
		else
			pages_count = 0;

		if (pages_count == num_of_pages) {
			uint32 start_of_allocation_index = i - (num_of_pages - 1);

			uint32 j;
			for (j = start_of_allocation_index;
					j < start_of_allocation_index + num_of_pages; j++) {
//				cprintf("malloc: page %d reserved for user \n",j);
				reservedPages[j] = num_of_pages;
			}
//			cprintf("malloc: #page %d reserved for user \n",j-start_of_allocation_index);
			start_of_allocation = start_of_allocation_index * PAGE_SIZE
					- USER_HEAP_START;
//			cprintf("malloc: sysCall allocate user mem va - %x \n",start_of_allocation);
			sys_allocate_user_mem(start_of_allocation, size);
			return (void *) start_of_allocation;
		}
	}
//	cprintf("malloc: out without get address\n");

	return NULL;

}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address) {
	//TODO: [PROJECT'24.MS2 - #14] [3] USER HEAP [USER SIDE] - free()
	// Write your code here, remove the panic and write your code

//    cprintf("free(): Freeing virtual address %x\n", (uint32)virtual_address);

	if (virtual_address == NULL) {
		panic("free(): NULL address provided");
	}

	if ((uint32) virtual_address >= USER_HEAP_START
			&& (uint32) virtual_address < myEnv->heapHardLimit) {

		free_block(virtual_address);
	} else if ((uint32) virtual_address >= myEnv->heapHardLimit + PAGE_SIZE
			&& (uint32) virtual_address < USER_HEAP_MAX) {

		uint32 va_index = ((uint32) virtual_address - USER_HEAP_START)
				/ PAGE_SIZE;
		uint32 num_pages = reservedPages[va_index];
		if (num_pages == 0) {
			panic("free(): Address not reserved in page allocator!");
		}

		for (uint32 i = va_index; i < va_index + num_pages; i++) {
			reservedPages[i] = 0;
		}

		sys_free_user_mem((uint32) virtual_address, num_pages * PAGE_SIZE);
	} else {

		panic("free(): Address is outside valid ranges");
	}
}

//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable) {
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	if (size == 0)
		return NULL;
	//==============================================================
	//TODO: [PROJECT'24.MS2 - #18] [4] SHARED MEMORY [USER SIDE] - smalloc()

	uint32 page_allocator_start = myEnv->heapHardLimit + PAGE_SIZE;
	uint32 page_allocator_end = USER_HEAP_MAX;
	uint32 num_of_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	uint32 start_of_allocation;
//	cprintf("smalloc: pages_count %d \n",num_of_pages);
	uint32 pages_count = 0;
	for (uint32 i = (page_allocator_start - USER_HEAP_START) / PAGE_SIZE;
			i < NUM_OF_UHEAP_PAGES; i++) {
		if (reservedPages[i] == 0)
			pages_count++;
		else
			pages_count = 0;

		if (pages_count == num_of_pages) {
			uint32 start_of_allocation_index = i - (num_of_pages - 1);

			uint32 j;
			for (j = start_of_allocation_index;
					j < start_of_allocation_index + num_of_pages; j++) {
//				cprintf("smalloc: page %d reserved for user \n",j);
				reservedPages[j] = num_of_pages;
			}
//			cprintf("smalloc: #page %d reserved for user \n",j-start_of_allocation_index);
			start_of_allocation = start_of_allocation_index * PAGE_SIZE
					- USER_HEAP_START;
//			cprintf("smalloc: sysCall allocate user mem va - %x \n",start_of_allocation);
			int ret = sys_createSharedObject(sharedVarName, size, isWritable,
					(void*) start_of_allocation);
			if (ret < 0) {
				for (j = start_of_allocation_index;
						j < start_of_allocation_index + num_of_pages; j++) {
//					cprintf("smalloc: page %d reserved for user \n",j);
					reservedPages[j] = 0;
				}
				return NULL;
			}
//			char* shareName, uint32 size, uint8 isWritable, void* virtual_address
			return (void *) start_of_allocation;
		}
	}

	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName) {
	//TODO: [PROJECT'24.MS2 - #20] [4] SHARED MEMORY [USER SIDE] - sget()
	uint32 size = sys_getSizeOfSharedObject(ownerEnvID, sharedVarName);
	if (size < 0) {
		return NULL;
	}
	uint32 page_allocator_start = myEnv->heapHardLimit + PAGE_SIZE;
	uint32 page_allocator_end = USER_HEAP_MAX;
	uint32 num_of_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
	uint32 start_of_allocation;
	uint32 pages_count = 0;
	//cprintf("sget: pages_count %d \n",num_of_pages);
	for (uint32 i = (page_allocator_start - USER_HEAP_START) / PAGE_SIZE;
			i < NUM_OF_UHEAP_PAGES; i++) {
		if (reservedPages[i] == 0)
			pages_count++;
		else
			pages_count = 0;

		if (pages_count == num_of_pages) {
			uint32 start_of_allocation_index = i - (num_of_pages - 1);

			uint32 j;
			for (j = start_of_allocation_index;
					j < start_of_allocation_index + num_of_pages; j++) {
//				cprintf("sget: page %d reserved for user \n",j);
				reservedPages[j] = num_of_pages;
			}
//			cprintf("sget: #page %d reserved for user \n",j-start_of_allocation_index);
			start_of_allocation = start_of_allocation_index * PAGE_SIZE
					- USER_HEAP_START;
//			cprintf("sget: sysCall allocate user mem va - %x \n",start_of_allocation);
			int ret = sys_getSharedObject(ownerEnvID, sharedVarName,
					(void*) start_of_allocation);
			if (ret < 0) {
				for (j = start_of_allocation_index;
						j < start_of_allocation_index + num_of_pages; j++) {
//					cprintf("sget: page %d reserved for user \n",j);
					reservedPages[j] = 0;
				}
				return NULL;
			}
//			char* shareName, uint32 size, uint8 isWritable, void* virtual_address
			return (void *) start_of_allocation;
		}
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
	uint32 page_start_index = ((uint32)virtual_address - USER_HEAP_START) / PAGE_SIZE;
	uint32 num_pages = reservedPages[page_start_index];

	struct Share* sharedObj = virtual_address;

//	int32 shareObjID = sharedObj->ID;
//
//	if(shareObjID == 0) {
//		cprintf("sfree: Failed to free the shared object (No shared Object)\n");
//		return;
//	}
//
//	int result = sys_freeSharedObject(shareObjID, virtual_address);
//
//	if (result != 0)
//	{
//		cprintf("sfree: Failed to free the shared object (Error Code: %d)\n", result);
//		return;
//	}

// 5. Mark the corresponding pages as free in the reservedPages array
//	uint32 num_pages = sys_getSizeOfSharedObjectByID(sharedObjID) / PAGE_SIZE;

	for (uint32 i = page_start_index; i < page_start_index + num_pages; i++)
	{
		reservedPages[i] = 0; // Mark page as free
	}

	cprintf("sfree: Successfully freed the shared object at %p\n", virtual_address);

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
void *realloc(void *virtual_address, uint32 new_size) {
	//[PROJECT]
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}

//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize) {
	panic("Not Implemented");

}
void shrink(uint32 newSize) {
	panic("Not Implemented");

}
void freeHeap(void* virtual_address) {
	panic("Not Implemented");

}
