/*
 * chunk_operations.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include <kern/trap/fault_handler.h>
#include <kern/disk/pagefile_manager.h>
#include <kern/proc/user_environment.h>
#include "kheap.h"
#include "memory_manager.h"
#include <inc/queue.h>
#include <inc/dynamic_allocator.h>

#define RESERVED_FOR_USER 0x00000100 //use available bits in virtual address to mark this page reserved without allocation in memory "8th bit"

//extern void inctst();

/******************************/
/*[1] RAM CHUNKS MANIPULATION */
/******************************/

//===============================
// 1) CUT-PASTE PAGES IN RAM:
//===============================
//This function should cut-paste the given number of pages from source_va to dest_va on the given page_directory
//	If the page table at any destination page in the range is not exist, it should create it
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, cut-paste the number of pages and return 0
//	ALL 12 permission bits of the destination should be TYPICAL to those of the source
//	The given addresses may be not aligned on 4 KB
int cut_paste_pages(uint32* page_directory, uint32 source_va, uint32 dest_va,
		uint32 num_of_pages) {
	//[PROJECT] [CHUNK OPERATIONS] cut_paste_pages
	// Write your code here, remove the panic and write your code
	panic("cut_paste_pages() is not implemented yet...!!");
}

//===============================
// 2) COPY-PASTE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va on the given page_directory
//	Ranges DO NOT overlapped.
//	If ANY of the destination pages exists with READ ONLY permission, deny the entire process and return -1.
//	If the page table at any destination page in the range is not exist, it should create it
//	If ANY of the destination pages doesn't exist, create it with the following permissions then copy.
//	Otherwise, just copy!
//		1. WRITABLE permission
//		2. USER/SUPERVISOR permission must be SAME as the one of the source
//	The given range(s) may be not aligned on 4 KB
int copy_paste_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va,
		uint32 size) {
	//[PROJECT] [CHUNK OPERATIONS] copy_paste_chunk
	// Write your code here, remove the //panic and write your code
	panic("copy_paste_chunk() is not implemented yet...!!");
}

//===============================
// 3) SHARE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va on the given page_directory
//	Ranges DO NOT overlapped.
//	It should set the permissions of the second range by the given perms
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, share the required range and return 0
//	If the page table at any destination page in the range is not exist, it should create it
//	The given range(s) may be not aligned on 4 KB
int share_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va,
		uint32 size, uint32 perms) {
	//[PROJECT] [CHUNK OPERATIONS] share_chunk
	// Write your code here, remove the //panic and write your code
	panic("share_chunk() is not implemented yet...!!");
}

//===============================
// 4) ALLOCATE CHUNK IN RAM:
//===============================
//This function should allocate the given virtual range [<va>, <va> + <size>) in the given address space  <page_directory> with the given permissions <perms>.
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, allocate the required range and return 0
//	If the page table at any destination page in the range is not exist, it should create it
//	Allocation should be aligned on page boundary. However, the given range may be not aligned.
int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms) {
	//[PROJECT] [CHUNK OPERATIONS] allocate_chunk
	// Write your code here, remove the //panic and write your code
	panic("allocate_chunk() is not implemented yet...!!");
}

//=====================================
// 5) CALCULATE ALLOCATED SPACE IN RAM:
//=====================================
void calculate_allocated_space(uint32* page_directory, uint32 sva, uint32 eva,
		uint32 *num_tables, uint32 *num_pages) {
	//[PROJECT] [CHUNK OPERATIONS] calculate_allocated_space
	// Write your code here, remove the panic and write your code
	panic("calculate_allocated_space() is not implemented yet...!!");
}

//=====================================
// 6) CALCULATE REQUIRED FRAMES IN RAM:
//=====================================
//This function should calculate the required number of pages for allocating and mapping the given range [start va, start va + size) (either for the pages themselves or for the page tables required for mapping)
//	Pages and/or page tables that are already exist in the range SHOULD NOT be counted.
//	The given range(s) may be not aligned on 4 KB
uint32 calculate_required_frames(uint32* page_directory, uint32 sva,
		uint32 size) {
	//[PROJECT] [CHUNK OPERATIONS] calculate_required_frames
	// Write your code here, remove the panic and write your code
	panic("calculate_required_frames() is not implemented yet...!!");
}

//=================================================================================//
//===========================END RAM CHUNKS MANIPULATION ==========================//
//=================================================================================//

/*******************************/
/*[2] USER CHUNKS MANIPULATION */
/*******************************/

//======================================================
/// functions used for USER HEAP (malloc, free, ...)
//======================================================
//=====================================
/* DYNAMIC ALLOCATOR SYSTEM CALLS */
//=====================================
void* sys_sbrk(int numOfPages) {
	/* numOfPages > 0: move the segment break of the current user program to increase the size of its heap
	 * 				by the given number of pages. You should allocate NOTHING,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * numOfPages = 0: just return the current position of the segment break
	 *
	 * NOTES:
	 * 	1) As in real OS, allocate pages lazily. While sbrk moves the segment break, pages are not allocated
	 * 		until the user program actually tries to access data in its heap (i.e. will be allocated via the fault handler).
	 * 	2) Allocating additional pages for a process’ heap will fail if, for example, the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sys_sbrk fails, the net effect should
	 * 		be that sys_sbrk returns (void*) -1 and that the segment break and the process heap are unaffected.
	 * 		You might have to undo any operations you have done so far in this case.
	 */
	//TODO: [PROJECT'24.MS2 - #11] [3] USER HEAP - sys_sbrk
	/*====================================*/
	/*Remove this line before start coding*/

	/*====================================*/
	struct Env* env = get_cpu_proc(); //the current running Environment to adjust its break limit

	if (numOfPages == 0) {
//		cprintf("sys_sbrk:  no modifie break is %x \n",env->heapBreak);
		return (void *) env->heapBreak;
	}
	if (numOfPages < 0) {
//		cprintf("sys_sbrk: Increment with negative value  \n");
		return (void *) -1;
	}

	//cprintf("sys_sbrk was called \n");

	uint32 oldBreak = env->heapBreak;
	uint32 newBreak = oldBreak + numOfPages * PAGE_SIZE;

//	cprintf("sys_sbrk: %d inc old break %x ,hard limit %x ,new break %x \n",numOfPages ,env->heapBreak ,env->heapHardLimit, env->heapBreak+numOfPages*PAGE_SIZE);

	if (numOfPages > 0) {
		if (oldBreak + numOfPages * PAGE_SIZE > env->heapHardLimit) {
//			cprintf("sys_sbrk: break exceed the limit  \n");
			return (void*) -1;
		}
		uint32 *page_table;

		int ret = get_page_table(env->env_page_directory, oldBreak,&page_table);
		if (ret == TABLE_NOT_EXIST) {

			create_page_table(env->env_page_directory, oldBreak);
		}
		pt_set_page_permissions(env->env_page_directory, oldBreak,RESERVED_FOR_USER | PERM_USER | PERM_WRITEABLE, 0);

		env->heapBreak += numOfPages * PAGE_SIZE;

//		cprintf("sys_sbrk:  new break modified successfully old break is %x \n",oldBreak);

//		cprintf("sys_sbrk: free blocks is %d \n" , LIST_SIZE(&freeBlocksList));

		*(uint32*) (newBreak - sizeof(int)) = 1;

		env->heapOldBreak = oldBreak;
		return (void*) oldBreak;

	} else {
//		cprintf("sys_sbrk: invalid increment old break is %x \n" , oldBreak);
		return (void*) -1;
	}



}

//=====================================
// 1) ALLOCATE USER MEMORY:
//=====================================
//
//uint32 setAsReserved(uint32 page_address) {
//	return (page_address | RESERVED_FOR_USER);
//}
//uint32 setAsUnreserved(uint32 page_address) {
//	return (page_address & ~RESERVED_FOR_USER);
//}
//bool isReserved(uint32 page_address) {
//	return (page_address & RESERVED_FOR_USER);
//}

void allocate_user_mem(struct Env* e, uint32 virtual_address, uint32 size) {

	/*====================================*/
	/*Remove this line before start coding*/
	//    inctst();
	//    return;
	/*====================================*/
	//TODO: [PROJECT'24.MS2 - #13] [3] USER HEAP [KERNEL SIDE] - allocate_user_mem()
	// Write your code here, remove the panic and write your code

//	cprintf("allocate_user_mem: allocating size %d at %x \n", size,virtual_address);

	uint32 num_of_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;

	uint32* page_table;


	for (uint32 current_address = virtual_address;
			current_address < virtual_address + num_of_pages * PAGE_SIZE;
			current_address += PAGE_SIZE) {

		int ret = get_page_table(e->env_page_directory, current_address,
				&page_table);

		if (ret == TABLE_NOT_EXIST) {

			create_page_table(e->env_page_directory, current_address);
		}
		pt_set_page_permissions(e->env_page_directory, current_address,
				RESERVED_FOR_USER | PERM_USER | ~PERM_PRESENT | PERM_WRITEABLE,
				0);
	}


}

//=====================================
// 2) FREE USER MEMORY:
//=====================================
void free_user_mem(struct Env* e, uint32 virtual_address, uint32 size) {
	/*====================================*/
	/*Remove this line before start coding*/
//	inctst();
//	return;
	/*====================================*/

	//TODO: [PROJECT'24.MS2 - #15] [3] USER HEAP [KERNEL SIDE] - free_user_mem
	// Write your code here, remove the panic and write your code
	//TODO: [PROJECT'24.MS2 - BONUS#3] [3] USER HEAP [KERNEL SIDE] - O(1) free_user_mem

    uint32 num_pages = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
    uint32 start_address = ROUNDDOWN((uint32)virtual_address, PAGE_SIZE);

    //cprintf("free_user_mem: Starting to free memory at VA = %x, size = %x\n", virtual_address, size);


    for (uint32 current_address = start_address; current_address < start_address + num_pages * PAGE_SIZE; current_address += PAGE_SIZE) {



        env_page_ws_invalidate(e,current_address);


        int ret_pf = pf_read_env_page(e, (void *)current_address);
        if (ret_pf != E_PAGE_NOT_EXIST_IN_PF) {
            pf_remove_env_page(e, current_address);
        }


        pt_set_page_permissions(e->env_page_directory, current_address, 0, RESERVED_FOR_USER | PERM_USER | ~PERM_PRESENT | PERM_WRITEABLE);

        unmap_frame(e->env_page_directory, current_address);
    }


//    cprintf("free_user_mem: Completed freeing memory range starting at VA = %x\n", virtual_address);
}



//=====================================
// 2) FREE USER MEMORY (BUFFERING):
//=====================================
void __free_user_mem_with_buffering(struct Env* e, uint32 virtual_address,
		uint32 size) {
	// your code is here, remove the panic and write your code
	panic("__free_user_mem_with_buffering() is not implemented yet...!!");
}

//=====================================
// 3) MOVE USER MEMORY:
//=====================================
void move_user_mem(struct Env* e, uint32 src_virtual_address,
		uint32 dst_virtual_address, uint32 size) {
	//[PROJECT] [USER HEAP - KERNEL SIDE] move_user_mem
	//your code is here, remove the panic and write your code
	panic("move_user_mem() is not implemented yet...!!");
}

//=================================================================================//
//========================== END USER CHUNKS MANIPULATION =========================//
//=================================================================================//

