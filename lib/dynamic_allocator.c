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
__inline__ uint32 get_block_size(void* va) {
	uint32 *curBlkMetaData = ((uint32 *) va - 1);
	return (*curBlkMetaData) & ~(0x1);
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
__inline__ int8 is_free_block(void* va) {
	uint32 *curBlkMetaData = ((uint32 *) va - 1);
	return (~(*curBlkMetaData) & 0x1);
}

//===========================
// 3) ALLOCATE BLOCK:
//===========================

void *alloc_block(uint32 size, int ALLOC_STRATEGY) {
	void *va = NULL;
	switch (ALLOC_STRATEGY) {
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

void print_blocks_list(struct MemBlock_LIST list) {
	cprintf("=========================================\n");
	struct BlockElement* blk;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", get_block_size(blk),
				is_free_block(blk));
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
void initialize_dynamic_allocator(uint32 daStart,uint32 initSizeOfAllocatedSpace) {
	//==================================================================================
	//DON'T CHANGE THESE LINES==========================================================
	//==================================================================================
	{
		if (initSizeOfAllocatedSpace % 2 != 0)
			initSizeOfAllocatedSpace++; //ensure it's multiple of 2
		if (initSizeOfAllocatedSpace == 0)
			return;
		is_initialized = 1;
	}
	//==================================================================================
	//==================================================================================

	//TODO: [PROJECT'24.MS1 - #04] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
	//panic("initialize_dynamic_allocator is not implemented yet");

	LIST_INIT(&freeBlocksList);

	*(uint32*) daStart = 1; //BeginOfHeap = 1 "allocated" , size=0

	*(uint32*) (daStart + initSizeOfAllocatedSpace - sizeof(uint32)) = 1; //EndOfHeap =1 "allocated",size = 0

	*(uint32*) (daStart + sizeof(uint32)) = initSizeOfAllocatedSpace - 2 * sizeof(uint32); //header -> Size of heap - begin & end

	struct BlockElement* freeblock = (struct BlockElement*) (daStart+ 2 * sizeof(uint32));

	*(uint32*) (daStart + initSizeOfAllocatedSpace - 2 * sizeof(uint32)) = initSizeOfAllocatedSpace - 2 * sizeof(int); //footer -> Size of heap - begin & end
//	cprintf("initialize_dynamic_allocator : free_block_list size is  %d \n" , LIST_SIZE(&freeBlocksList));


	LIST_INSERT_TAIL(&freeBlocksList, freeblock);
//	cprintf("initialize_dynamic_allocator : free_block_list size is  %d \n" , LIST_SIZE(&freeBlocksList));

}
//==================================
// [2] SET BLOCK HEADER & FOOTER:
//==================================
void set_block_data(void* va, uint32 totalSize, bool isAllocated) {
//TODO: [PROJECT'24.MS1 - #05] [3] DYNAMIC ALLOCATOR - set_block_data
//COMMENT THE FOLLOWING LINE BEFORE START CODING
//panic("set_block_data is not implemented yet");
//Your Code is Here...
	va = va - sizeof(int);
	*(uint32*) va = totalSize + isAllocated;
	va = (void*) (va + totalSize - sizeof(uint32));
	*(uint32*) va = totalSize + isAllocated;

}

//=========================================
// [3] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size) {
//==================================================================================
//DON'T CHANGE THESE LINES==========================================================
//==================================================================================
	{
		if (size % 2 != 0)
			size++; //ensure that the size is even (to use LSB as allocation flag)
		if (size < DYN_ALLOC_MIN_BLOCK_SIZE)
			size = DYN_ALLOC_MIN_BLOCK_SIZE;
		if (!is_initialized) {
			uint32 required_size = size + 2 * sizeof(uint32) /*header & footer*/
			+ 2 * sizeof(uint32) /*da begin & end*/;
			uint32 da_start = (uint32) sbrk(
			ROUNDUP(required_size, PAGE_SIZE) / PAGE_SIZE);
			uint32 da_break = (uint32) sbrk(0);
//			cprintf("allocate_FF: dastart %x  dabreak %x\n" ,da_start, da_break);

			initialize_dynamic_allocator(da_start, da_break - da_start);
		}
	}
//==================================================================================
//==================================================================================

//TODO: [PROJECT'24.MS1 - #06] [3] DYNAMIC ALLOCATOR - alloc_block_FF
//COMMENT THE FOLLOWING LINE BEFORE START CODING
//panic("alloc_block_FF is not implemented yet");

	void *va=NULL;
	bool find = 0;
	bool sbrkBool = 0;

	if (size == 0) {
		return NULL;
	}

//	cprintf("allocate_FF: allocating size %d \n" , size);
//	cprintf("allocate_FF: free_block_list size is  %d \n" , LIST_SIZE(&freeBlocksList));


	struct BlockElement* blk;
	x:
	blk=NULL;

	LIST_FOREACH(blk, &freeBlocksList)
	{



		uint32 blockSize = get_block_size(blk);

		if (blockSize >= (size + 2 * sizeof(uint32))) {



			if ((blockSize - (size + 2 * sizeof(uint32))) < 4 * sizeof(uint32)) //INTERNAL FRAGMENTATION
					{

				va = blk;
				set_block_data(va, blockSize, 1);
				LIST_REMOVE(&freeBlocksList, blk);


			}

			else {

				va = blk;
				void* splitedBlock_va = va + size + 8;
				set_block_data(blk, size + 8, 1);
				set_block_data(splitedBlock_va, blockSize - (size + 8), 0);

				struct BlockElement* splitedBlock = splitedBlock_va;

				LIST_INSERT_AFTER(&freeBlocksList, blk, splitedBlock);
				LIST_REMOVE(&freeBlocksList, blk);


			}

			return va;
		}

	}
	if (!sbrkBool) {

		uint32 required_size = size + 2 * sizeof(uint32);
		uint32 num_pages = ROUNDUP(required_size, PAGE_SIZE) / PAGE_SIZE;

		void *new_break = sbrk(num_pages);
//		cprintf("allocate_FF: Sbrk Returned %x \n" ,new_break);
		if (new_break == (void *) -1) {
//			cprintf("allocate_FF: Sbrk Fail \n" );

			return NULL;
		}

		uint32 new_block_size = num_pages * PAGE_SIZE;

		uint32 new_end_block = (uint32) new_break + new_block_size;

		uint32 lastBlockFooter = (uint32) new_break - sizeof(uint32);
		if (is_free_block((void *) lastBlockFooter) && get_block_size((void *) lastBlockFooter)) {
//			cprintf("allocate_FF: merge last block with new sbrk block is  %x \n" , new_break);

			struct BlockElement *lastBlock = (void *) (lastBlockFooter
					- get_block_size((void *) lastBlockFooter) + sizeof(uint32));
			set_block_data(lastBlock,
					get_block_size(lastBlock) + new_block_size, 0);

		} else {
//			cprintf("allocate_FF: allocate new sbrk block is  %x \n" , new_break);

			set_block_data(new_break, new_block_size, 0);
			struct BlockElement *newBlock = new_break;
			LIST_INSERT_TAIL(&freeBlocksList, newBlock);

		}
//		cprintf("allocate_FF: free_block_list size is  %d \n" , LIST_SIZE(&freeBlocksList));

		sbrkBool = 1;
//		cprintf("allocate_FF: Retrying the allocation process \n" );

		goto x;
	}

	return NULL;
}
//=========================================
// [4] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size) {
	//TODO: [PROJECT'24.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF
	//COMMENT THE FOLLOWING LINE BEFORE START CODING

	void *va;
	bool find = 0;
	bool sbrkBool = 0;

	if (size == 0) {
		return NULL;
	}
	uint32 min_size = (size + 2 * sizeof(uint32));

	struct BlockElement* blk = NULL;
	struct BlockElement* bl;
	x:
	LIST_FOREACH(bl, &freeBlocksList)
	{
		uint32 block = get_block_size(bl);

		if (block >= min_size && (blk == NULL || block < get_block_size(blk))) {
			blk = bl;
			va = blk;
		}

	}
	uint32 blockSize = get_block_size(blk);

	if (blockSize >= (size + 2 * sizeof(uint32))) {
		if ((blockSize - (size + 2 * sizeof(uint32))) < 4 * sizeof(uint32)) { //INTERNAL FRAGMENTATION
			set_block_data(va, blockSize, 1);
			LIST_REMOVE(&freeBlocksList, blk);
		} else {
			void* splitedBlock_va = va + size + 8;
			set_block_data(blk, size + 8, 1);
			set_block_data(splitedBlock_va, blockSize - (size + 8), 0);
			struct BlockElement* splitedBlock = splitedBlock_va;
			LIST_INSERT_AFTER(&freeBlocksList, blk, splitedBlock);
			LIST_REMOVE(&freeBlocksList, blk);
		}
		return va;
	}

	if (!sbrkBool) {
		sbrk(1);
		sbrkBool = 1;
		goto x;
	}

	return NULL;
}

//===================================================
// [5] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va) {
	//TODO: [PROJECT'24.MS1 - #07] [3] DYNAMIC ALLOCATOR - free_block
	//COMMENT THE FOLLOWING LINE BEFORE START CODING
//	panic("free_block is not implemented yet");
	//Your Code is Here...

	if (is_free_block(va)) {
		return;
	}

	uint32 block_size = get_block_size(va);
	if (block_size == 0) {
		return;
	}

	void *orignal_va = va;
	set_block_data(orignal_va, block_size, 0);

	uint32 *endOfPrevBlockFooter = (uint32*) va - 1;
	bool is_prev_free = is_free_block(endOfPrevBlockFooter);

	uint32 prev_block_size = get_block_size(endOfPrevBlockFooter);
	void *prevBlock_va = va - prev_block_size;

	void *nextBlock_va = va + block_size;
	bool is_nxt_free = is_free_block(nextBlock_va);
	uint32 nxt_block_Size = get_block_size(nextBlock_va);

	if (block_size != 0) {

		if (is_prev_free && is_nxt_free) {
			uint32 total_size = prev_block_size + block_size + nxt_block_Size;

			set_block_data(prevBlock_va, total_size, 0);

			LIST_REMOVE(&freeBlocksList, (struct BlockElement* ) nextBlock_va);

			set_block_data(orignal_va, 0, 0);

			return;
		}

		if (is_prev_free) {
			uint32 total_size = prev_block_size + block_size;

			set_block_data(prevBlock_va, total_size, 0);

			set_block_data(orignal_va, 0, 0);

			return;
		}

		if (is_nxt_free) {
			uint32 total_size = block_size + nxt_block_Size;

			set_block_data(orignal_va, total_size, 0);
			LIST_INSERT_BEFORE(&freeBlocksList, (struct BlockElement* ) nextBlock_va, (struct BlockElement*) orignal_va);
			LIST_REMOVE(&freeBlocksList, (struct BlockElement* ) nextBlock_va);

			return;
		}

		struct BlockElement* freeblock = (struct BlockElement*) orignal_va;

		struct BlockElement* block;
		LIST_FOREACH(block, &freeBlocksList)
		{
			if ((uint32) block > (uint32) freeblock) {
				set_block_data(orignal_va, block_size, 0);
				LIST_INSERT_BEFORE(&freeBlocksList, block, freeblock);

				return;
			}
		}

		set_block_data(orignal_va, block_size, 0);
		LIST_INSERT_TAIL(&freeBlocksList, freeblock);

	}

}

//=========================================
// [6] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size) {
	// TODO: [PROJECT'24.MS1 - #08] [3] DYNAMIC ALLOCATOR - realloc_block_FF
	// COMMENT THE FOLLOWING LINE BEFORE START CODING
	uint32 block_size = get_block_size(va);
	uint32 total_size = new_size + 2 * sizeof(uint32);

	if (new_size == 0) {
		free_block(va);
		return NULL;
	} else if (va == NULL) {
		return alloc_block_FF(new_size);
	} else if (block_size == total_size) {
		return va;
	}

	if (total_size > block_size) {
		void* nextBlock_va = va + block_size;
		uint32 nxt_block_Size = get_block_size(nextBlock_va);
		bool is_nxt_free = is_free_block(nextBlock_va);
		uint32 merge_size = block_size + nxt_block_Size;

		if (is_nxt_free && total_size <= merge_size) {

			if (merge_size - total_size > 16) {
				// merge and split addition block
				//set_block_data(va, total_size, 1);
				void* splitedBlock_va = (char *) va + new_size + 8;
				set_block_data(va, new_size + 8, 1);
				set_block_data(splitedBlock_va, merge_size - total_size, 0);
				free_block(splitedBlock_va);
				return va;
			} else {
				LIST_REMOVE(&freeBlocksList,
						(struct BlockElement * )nextBlock_va); // Remove from free list
				set_block_data(va, merge_size, 1); // Update header size after merging
				return va; // Return original block pointer
			}
		}

		else {
			set_block_data(va, total_size, 1);
			return va;
		}
	} else if (total_size < block_size) {
		if (block_size - total_size > 16) {
			// split addition block
			void* splitedBlock_va = (char *) va + new_size + 8;
			set_block_data(va, new_size + 8, 1);
			set_block_data(splitedBlock_va, block_size - total_size, 1);
			free_block(splitedBlock_va);
			return va;
		} else {
			return va;
		}
	}
	return NULL;
}

/*********************************************************************************************/
/*********************************************************************************************/
/*********************************************************************************************/
//=========================================
// [7] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size) {
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [8] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size) {
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}
