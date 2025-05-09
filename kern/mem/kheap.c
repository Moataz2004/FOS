#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
//All pages in the given range should be allocated
//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
//Return:
//	On success: 0
//	Otherwise (if no memory OR initial size exceed the given limit): PANIC
int initialize_kheap_dynamic_allocator(uint32 daStart,
		uint32 initSizeToAllocate, uint32 daLimit) {
	//TODO: [PROJECT'24.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator
	// Write your code here, remove the panic and write your code
//	panic("initialize_kheap_dynamic_allocator() is not implemented yet...!!");
	init_spinlock(&klock,"Lock");
	if (initSizeToAllocate > daLimit
			|| daStart + initSizeToAllocate > daLimit) {
		panic("Initialization exceeds kernel heap limit");
		return -1;
	}

	kheap_start = daStart;
	kheap_break = daStart + initSizeToAllocate;
	kheap_hard_limit = daLimit;

	for (uint32 addr = kheap_start; addr < kheap_break; addr += PAGE_SIZE) {
		struct FrameInfo *frame;
		int result = allocate_frame(&frame);
		if (result != 0
				|| map_frame(ptr_page_directory, frame, addr, PERM_WRITEABLE)
						!= 0) {
			panic("Failed to map initial kernel heap pages");
			return -1;
		}
		frame->va = addr;
	}
	initialize_dynamic_allocator(kheap_start, initSizeToAllocate);
	return 0;
}

void* sbrk(int numOfPages) {
	/* numOfPages > 0: move the segment break of the kernel to increase the size of its heap by the given numOfPages,
	 * 				you should allocate pages and map them into the kernel virtual address space,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * numOfPages = 0: just return the current position of the segment break
	 *
	 * NOTES:
	 * 	1) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, return -1
	 */
	//====================================================

	//TODO: [PROJECT'24.MS2 - #02] [1] KERNEL HEAP - sbrk
	// Write your code here, remove the panic and write your code
    // panic("sbrk() is not implemented yet...!!");

	if (numOfPages == 0) {
		return (void *) kheap_break;
	}
	if (numOfPages < 0) {
		return (void *) -1;
	}

	uint32 increment = numOfPages * PAGE_SIZE;
	uint32 new_break = kheap_break + increment;

	if (new_break > kheap_hard_limit) {
		return (void *) -1;
	}

	uint32 addr = kheap_break;
	while (addr < new_break) {
		struct FrameInfo *frame = NULL;
		if (allocate_frame(&frame) != 0
				|| map_frame(ptr_page_directory, frame, addr,
				PERM_WRITEABLE | PERM_PRESENT) != 0) {
			return (void *) -1;
		}
		frame->va = addr;
		addr += PAGE_SIZE;
	}

	uint32 old_break = kheap_break;
	kheap_break = new_break;
	*(uint32*) (new_break - sizeof(int)) = 1;
	return (void *) old_break;


}

//TODO: [PROJECT'24.MS2 - BONUS#2] [1] KERNEL HEAP - Fast Page Allocator

void* kmalloc(uint32 size) {
	//TODO: [PROJECT'24.MS2 - #03] [1] KERNEL HEAP - kmalloc
	// Write your code here, remove the panic and write your code
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
acquire_spinlock(&klock);
	uint32 limit_kmalloc = KERNEL_HEAP_MAX;
	uint32 start_kmalloc = kheap_hard_limit + PAGE_SIZE;

	if (size <= DYN_ALLOC_MAX_BLOCK_SIZE  && isUHeapPlacementStrategyFIRSTFIT()) {

		void *block = alloc_block_FF(size);
		release_spinlock(&klock);
		return block;
	} else {
		uint32 num_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
		uint32 count_pages = 0;
		uint32 alloc_start = 0;
		uint32* page_table;



		for (uint32 cur_addr = start_kmalloc; cur_addr < limit_kmalloc; cur_addr += PAGE_SIZE) {
			struct FrameInfo* check_frame = get_frame_info(ptr_page_directory,
					cur_addr, &page_table);

			if (check_frame != NULL) {

				count_pages = 0;
				continue;
			} else {
				count_pages++;
			}

			if (count_pages == num_pages) {

				alloc_start = cur_addr - (num_pages - 1) * PAGE_SIZE;

				for (uint32 alloc_addr = alloc_start;alloc_addr < alloc_start + num_pages * PAGE_SIZE;alloc_addr += PAGE_SIZE) {
					struct FrameInfo* new_frame;
					allocate_frame(&new_frame);
					map_frame(ptr_page_directory, new_frame, alloc_addr,PERM_PRESENT | PERM_WRITEABLE);
					new_frame->va = alloc_addr;

					new_frame->pages = num_pages;
				}
				release_spinlock(&klock);
				return (void*) alloc_start;
			}
		}
	}

//	cprintf("kmalloc: Failed to allocate 0x%x bytes\n", size);
	release_spinlock(&klock);
	return NULL;
}

void kfree(void* virtual_address) {
	//TODO: [PROJECT'24.MS2 - #04] [1] KERNEL HEAP - kfree
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details
	acquire_spinlock(&klock);
	if (virtual_address == NULL) {
		return;
	}
	uint32 va = (uint32) virtual_address;

	if (va >= KERNEL_HEAP_START && va < kheap_hard_limit) {
		release_spinlock(&klock);
	return free_block(virtual_address);


	} else if (va >= kheap_hard_limit + PAGE_SIZE && va < KERNEL_HEAP_MAX) {
		uint32 *ptr_page_table = NULL;
		struct FrameInfo* frame_to_free = get_frame_info(ptr_page_directory,
				(uint32) virtual_address, &ptr_page_table);

		if (frame_to_free == NULL || frame_to_free->pages == 0) {
			release_spinlock(&klock);
			return;
		}

		uint32 no_pages = frame_to_free->pages;
		uint32 start = ROUNDDOWN((uint32 )virtual_address, PAGE_SIZE);

		for (uint32 i = 0; i < no_pages; i++) {
			uint32 *ptr_page_table = NULL;
			uint32 va = start + i * PAGE_SIZE;

			unmap_frame(ptr_page_directory, va);

		}

	} else {
		panic("kfree: Address outside valid kernel heap ranges");
	}
	release_spinlock(&klock);
	return;
}

unsigned int kheap_physical_address(unsigned int virtual_address) {
//TODO: [PROJECT'24.MS2 - #05] [1] KERNEL HEAP - kheap_physical_address
// Write your code here, remove the panic and write your code
//panic("kheap_physical_address() is not implemented yet...!!");

//return the physical address corresponding to given virtual_address
//refer to the project presentation and documentation for details

//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	uint32 *ptr_page_table = NULL;
	struct FrameInfo *ptr_frame_info = get_frame_info(ptr_page_directory,
			virtual_address, &ptr_page_table);
	if (ptr_frame_info) {
		uint32 table_entry = ptr_page_table[PTX(virtual_address)];
		int frameNum = table_entry >> 12;
		return (frameNum * PAGE_SIZE) + (virtual_address & 0xfff);
	}
	return 0;

}

unsigned int kheap_virtual_address(unsigned int physical_address) {
//TODO: [PROJECT'24.MS2 - #06] [1] KERNEL HEAP - kheap_virtual_address
// Write your code here, remove the panic and write your code
//panic("kheap_virtual_address() is not implemented yet...!!");

//return the virtual address corresponding to given physical_address
//refer to the project presentation and documentation for details

//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	struct FrameInfo *ptr_frame_info = to_frame_info(physical_address);
	uint32 *ptr_page_table = NULL;
	ptr_frame_info = get_frame_info(ptr_page_directory, ptr_frame_info->va,
			&ptr_page_table);
	if (ptr_frame_info == NULL) {
		return 0;
	}

	return (ptr_frame_info->va) + (physical_address & 0xfff);

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

void *krealloc(void *virtual_address, uint32 new_size) {
//TODO: [PROJECT'24.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc
// Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}
