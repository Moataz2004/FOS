/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <kern/cpu/cpu.h>
#include <kern/disk/pagefile_manager.h>
#include <kern/mem/memory_manager.h>

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE) {
	assert(
			LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE;
}
void setPageReplacmentAlgorithmCLOCK() {
	_PageRepAlgoType = PG_REP_CLOCK;
}
void setPageReplacmentAlgorithmFIFO() {
	_PageRepAlgoType = PG_REP_FIFO;
}
void setPageReplacmentAlgorithmModifiedCLOCK() {
	_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;
}
/*2018*/void setPageReplacmentAlgorithmDynamicLocal() {
	_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;
}
/*2021*/void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps) {
	_PageRepAlgoType = PG_REP_NchanceCLOCK;
	page_WS_max_sweeps = PageWSMaxSweeps;
}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE) {
	return _PageRepAlgoType == LRU_TYPE ? 1 : 0;
}
uint32 isPageReplacmentAlgorithmCLOCK() {
	if (_PageRepAlgoType == PG_REP_CLOCK)
		return 1;
	return 0;
}
uint32 isPageReplacmentAlgorithmFIFO() {
	if (_PageRepAlgoType == PG_REP_FIFO)
		return 1;
	return 0;
}
uint32 isPageReplacmentAlgorithmModifiedCLOCK() {
	if (_PageRepAlgoType == PG_REP_MODIFIEDCLOCK)
		return 1;
	return 0;
}
/*2018*/uint32 isPageReplacmentAlgorithmDynamicLocal() {
	if (_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL)
		return 1;
	return 0;
}
/*2021*/uint32 isPageReplacmentAlgorithmNchanceCLOCK() {
	if (_PageRepAlgoType == PG_REP_NchanceCLOCK)
		return 1;
	return 0;
}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt) {
	_EnableModifiedBuffer = enableIt;
}
uint8 isModifiedBufferEnabled() {
	return _EnableModifiedBuffer;
}

void enableBuffering(uint32 enableIt) {
	_EnableBuffering = enableIt;
}
uint8 isBufferingEnabled() {
	return _EnableBuffering;
}

void setModifiedBufferLength(uint32 length) {
	_ModifiedBufferLength = length;
}
uint32 getModifiedBufferLength() {
	return _ModifiedBufferLength;
}

//===============================
// FAULT HANDLERS
//===============================

//==================
// [1] MAIN HANDLER:
//==================
/*2022*/
uint32 last_eip = 0;
uint32 before_last_eip = 0;
uint32 last_fault_va = 0;
uint32 before_last_fault_va = 0;
int8 num_repeated_fault = 0;
#define RESERVED_FOR_USER 0x00000100 //use available bits in virtual address to mark this page reserved without allocation in memory "8th bit"
#define PERM_CHANCE 0x00000008
struct Env* last_faulted_env = NULL;
void fault_handler(struct Trapframe *tf) {
	/******************************************************/
	// Read processor's CR2 register to find the faulting address
	uint32 fault_va = rcr2();
	//	cprintf("\n************Faulted VA = %x************\n", fault_va);
	//	print_trapframe(tf);
	/******************************************************/

	//If same fault va for 3 times, then panic
	//UPDATE: 3 FAULTS MUST come from the same environment (or the kernel)
	struct Env* cur_env = get_cpu_proc();
	if (last_fault_va == fault_va && last_faulted_env == cur_env) {
		num_repeated_fault++;
		if (num_repeated_fault == 3) {
			print_trapframe(tf);
			panic(
					"Failed to handle fault! fault @ at va = %x from eip = %x causes va (%x) to be faulted for 3 successive times\n",
					before_last_fault_va, before_last_eip, fault_va);
		}
	} else {
		before_last_fault_va = last_fault_va;
		before_last_eip = last_eip;
		num_repeated_fault = 0;
	}
	last_eip = (uint32) tf->tf_eip;
	last_fault_va = fault_va;
	last_faulted_env = cur_env;
	/******************************************************/
	//2017: Check stack overflow for Kernel
	int userTrap = 0;
	if ((tf->tf_cs & 3) == 3) {
		userTrap = 1;
	}
	if (!userTrap) {
		struct cpu* c = mycpu();
		//cprintf("trap from KERNEL\n");
		if (cur_env
				&& fault_va
						>= (uint32) cur_env->kstack&& fault_va < (uint32)cur_env->kstack + PAGE_SIZE)
			panic("User Kernel Stack: overflow exception!");
		else if (fault_va
				>= (uint32) c->stack&& fault_va < (uint32)c->stack + PAGE_SIZE)
			panic("Sched Kernel Stack of CPU #%d: overflow exception!",
					c - CPUS);
#if USE_KHEAP
		if (fault_va >= KERNEL_HEAP_MAX)
			panic("Kernel: heap overflow exception!");
#endif
	}
	//2017: Check stack underflow for User
	else {
		//cprintf("trap from USER\n");
		if (fault_va >= USTACKTOP && fault_va < USER_TOP)
			panic("User: stack underflow exception!");
	}

	//get a pointer to the environment that caused the fault at runtime
	//cprintf("curenv = %x\n", curenv);
	struct Env* faulted_env = cur_env;
	if (faulted_env == NULL) {
		print_trapframe(tf);
		panic("faulted env == NULL!");
	}
	//check the faulted address, is it a table or not ?
	//If the directory entry of the faulted address is NOT PRESENT then
	if ((faulted_env->env_page_directory[PDX(fault_va)] & PERM_PRESENT)
			!= PERM_PRESENT) {
		// we have a table fault =============================================================
		//		cprintf("[%s] user TABLE fault va %08x\n", curenv->prog_name, fault_va);
		//		print_trapframe(tf);

		faulted_env->tableFaultsCounter++;

		table_fault_handler(faulted_env, fault_va);
	} else {
		if (userTrap) {
			/*============================================================================================*/
			//TODO: [PROJECT'24.MS2 - #08] [2] FAULT HANDLER I - Check for invalid pointers
			//(e.g. pointing to unmarked user heap page, kernel or wrong access rights),
			//your code is here
			int perms = pt_get_page_permissions(faulted_env->env_page_directory,
					fault_va);

			if (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX) {
				if ((perms & RESERVED_FOR_USER) != RESERVED_FOR_USER) {
					cprintf(
							"Invalid pointer fault: Page is not Marked as reserved for user at VA = %x\n",
							fault_va);
					env_exit();
				}
				if ((perms & PERM_USER) != PERM_USER) {
					cprintf(
							"Invalid pointer fault: Page not accessible to user at VA = %x\n",
							fault_va);
					env_exit();
				}
			}

			else if (fault_va >= KERNEL_HEAP_START && fault_va < KERNEL_HEAP_MAX) {
				cprintf(
						"Invalid pointer fault: Attempt to access kernel memory at VA = %x\n",
						fault_va);
				env_exit();
			} else if ((perms & PERM_PRESENT)
					&& (perms & PERM_USER) != PERM_USER) {
				cprintf(
						"Invalid pointer fault: Page not accessible to user at VA = %x\n",
						fault_va);
				env_exit();
			} else if ((perms & PERM_PRESENT)
					&& (perms & PERM_WRITEABLE) != PERM_WRITEABLE) {
				cprintf(
						"Invalid pointer fault: Attempt to access Not WRITEABLE Page at VA = %x\n",
						fault_va);
				env_exit();
			}

			/*============================================================================================*/
		}

		/*2022: Check if fault due to Access Rights */
		int perms = pt_get_page_permissions(faulted_env->env_page_directory,
				fault_va);
		if (perms & PERM_PRESENT)
			panic(
					"Page @va=%x is exist! page fault due to violation of ACCESS RIGHTS\n",
					fault_va);
		/*============================================================================================*/

		// we have normal page fault =============================================================
		faulted_env->pageFaultsCounter++;

		//		cprintf("[%08s] user PAGE fault va %08x\n", curenv->prog_name, fault_va);
		//		cprintf("\nPage working set BEFORE fault handler...\n");
		//		env_page_ws_print(curenv);

		if (isBufferingEnabled()) {
			__page_fault_handler_with_buffering(faulted_env, fault_va);
		} else {
			//page_fault_handler(faulted_env, fault_va);
			page_fault_handler(faulted_env, fault_va);
		}
		//		cprintf("\nPage working set AFTER fault handler...\n");
		//		env_page_ws_print(curenv);

	}

	/*************************************************************/
	//Refresh the TLB cache
	tlbflush();
	/*************************************************************/
}

//=========================
// [2] TABLE FAULT HANDLER:
//=========================
void table_fault_handler(struct Env * curenv, uint32 fault_va) {
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory,
				(uint32) fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}
void replacementN(struct Env * faulted_env, uint32 fault_va, int perms) {
	uint32 *ptr_page_table = NULL;

	get_page_table(faulted_env->env_page_directory, faulted_env->page_last_WS_element->virtual_address, &ptr_page_table);

	struct FrameInfo *ptr_frame_info = get_frame_info(
			faulted_env->env_page_directory,
			faulted_env->page_last_WS_element->virtual_address,
			&ptr_page_table);

	if (((perms & PERM_MODIFIED) == PERM_MODIFIED)) {
		pf_update_env_page(faulted_env,
				faulted_env->page_last_WS_element->virtual_address,
				ptr_frame_info);
	}

	struct FrameInfo *new_frame_info;

	if (allocate_frame(&new_frame_info) != 0
			|| map_frame(faulted_env->env_page_directory, new_frame_info,
					fault_va,
					PERM_USER | PERM_WRITEABLE ) != 0) {
		env_exit();
	}
	unmap_frame(faulted_env->env_page_directory, faulted_env->page_last_WS_element->virtual_address);



	if (pf_read_env_page(faulted_env,
			(void *) fault_va) == E_PAGE_NOT_EXIST_IN_PF) {

		if ((fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)) {
			// Valid stack page
		} else if ((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)) {
			// Valid heap page
		} else {
			env_exit();
		}
	}
	faulted_env->page_last_WS_element->virtual_address = fault_va;
}
//=========================
// [3] PAGE FAULT HANDLER:
//=========================
void page_fault_handler(struct Env * faulted_env, uint32 fault_va) {
#if USE_KHEAP
	struct WorkingSetElement *victimWSElement = NULL;
	uint32 wsSize = LIST_SIZE(&(faulted_env->page_WS_list));
#else
	int iWS =faulted_env->page_last_WS_index;
	uint32 wsSize = env_page_ws_get_size(faulted_env);
#endif

	if (wsSize < faulted_env->page_WS_max_size) {
		//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
		//TODO: [PROJECT'24.MS2 - #09] [2] FAULT HANDLER I - Placement
		// Write your code here, remove the panic and write your code

		struct FrameInfo *ptr_frame_info = NULL;

		int ret_frame = allocate_frame(&ptr_frame_info);
		if (ret_frame == E_NO_MEM) {

			cprintf(
					"page_fault_handler Error: No free frames available for VA = %x\n",
					fault_va);
			env_exit();
		}

		if (map_frame(faulted_env->env_page_directory, ptr_frame_info, fault_va,
		PERM_USER | PERM_WRITEABLE) != 0) {
			cprintf(
					"page_fault_handler Error: Failed to map frame for VA = %x\n",
					fault_va);
			env_exit();
		}

		int ret_read = pf_read_env_page(faulted_env, (void *) fault_va);
		if (ret_read == E_PAGE_NOT_EXIST_IN_PF) {

			if ((fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)) {
				//				cprintf(
				//						"page_fault_handler: address = %x not exist in PAGE_FILE but in STACK \n",
				//						fault_va);
			} else if ((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)) {
				//				cprintf(
				//						"page_fault_handler: address = %x not exist in PAGE_FILE but in HEAP \n",
				//						fault_va);

			} else {
				cprintf(
						"page_fault_handler: address = %x not exist in PAGE_FILE and not in UHEAP/UStack \n",
						fault_va);
				env_exit();
			}
		}

		struct WorkingSetElement *new_element = env_page_ws_list_create_element(
				faulted_env, fault_va);
		if (new_element == NULL) {
			cprintf(
					"page_fault_handler Error: Failed to create working set element for VA = %x\n",
					fault_va);
			env_exit();
		}

		LIST_INSERT_TAIL(&faulted_env->page_WS_list, new_element);

		if (LIST_SIZE(&faulted_env->page_WS_list)
				== faulted_env->page_WS_max_size) {
			faulted_env->page_last_WS_element = LIST_FIRST(
					&faulted_env->page_WS_list);
		} else {
			faulted_env->page_last_WS_element = NULL;
		}

	} else {
		//TODO: [PROJECT�24.MS3 - #1] [1] PAGE FAULT HANDLER � Nth CLK Replace.

		int max_sweeps = 0;
		bool isModifiedV = 0;
		if (page_WS_max_sweeps >= 0) {
			max_sweeps = page_WS_max_sweeps;

		} else {
			max_sweeps = page_WS_max_sweeps * -1;
			isModifiedV = 1;
		}

		// Replacement logic for Nth Clock
//		struct WorkingSetElement* last_WS_element =
//				faulted_env->page_last_WS_element;
		while (1 == 1) {

			uint32 va = faulted_env->page_last_WS_element->virtual_address;
			int perms = pt_get_page_permissions(faulted_env->env_page_directory,
					va);

			if (perms & PERM_USED) {
				pt_set_page_permissions(faulted_env->env_page_directory, va, 0,
				PERM_USED);
				faulted_env->page_last_WS_element->sweeps_counter = 0;
			} else {
				faulted_env->page_last_WS_element->sweeps_counter++;

				if (!isModifiedV) {
					if (faulted_env->page_last_WS_element->sweeps_counter == max_sweeps) {
						faulted_env->page_last_WS_element->sweeps_counter = 0;
						replacementN(faulted_env, fault_va, perms);
						break;
					}
				} else {
					if (faulted_env->page_last_WS_element->sweeps_counter == max_sweeps) {

						if (!(perms & PERM_MODIFIED)) {
							faulted_env->page_last_WS_element->sweeps_counter = 0;
							replacementN(faulted_env, fault_va, perms);
							break;
						}
						else {
//							continue;
						}

					}
					if (faulted_env->page_last_WS_element->sweeps_counter == max_sweeps + 1
							&& (perms & PERM_MODIFIED)) {
						faulted_env->page_last_WS_element->sweeps_counter = 0;
						replacementN(faulted_env, fault_va, perms);
						break;
					}
				}

			}
			if (LIST_NEXT(faulted_env->page_last_WS_element) == NULL) {
				faulted_env->page_last_WS_element = LIST_FIRST(&(faulted_env->page_WS_list));
			} else
				faulted_env->page_last_WS_element = LIST_NEXT(faulted_env->page_last_WS_element);

//			env_page_ws_print(faulted_env);
		}
		if (LIST_NEXT(faulted_env->page_last_WS_element) == NULL) {
			faulted_env->page_last_WS_element = LIST_FIRST(&(faulted_env->page_WS_list));
		} else
			faulted_env->page_last_WS_element = LIST_NEXT(faulted_env->page_last_WS_element);

	}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va) {
//[PROJECT] PAGE FAULT HANDLER WITH BUFFERING
// your code is here, remove the panic and write your code
	panic("__page_fault_handler_with_buffering() is not implemented yet...!!");
}

