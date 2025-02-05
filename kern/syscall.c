/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>
#include <kern/time.h>
#include "e1000.h"

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	user_mem_assert(curenv, s, len, PTE_U);
	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, true)) < 0)
		return r;
//	if (e == curenv)
//		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
//	else
//		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);

	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.

	struct Env *env;
	int ret = env_alloc(&env, curenv->env_id);
	if (ret < 0)
		return ret;

	env->env_tf = curenv->env_tf;
	env->env_status = ENV_NOT_RUNNABLE;
	env->env_tf.tf_regs.reg_eax = 0;

	return env->env_id;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE)
		return -E_INVAL;
	struct Env *env;
	int ret = envid2env(envid, &env, true);
	if (ret < 0)
		return ret;
	env->env_status = status;
	return 0;
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3), interrupts enabled, and IOPL of 0.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	// LAB 5: Your code here.
	// Remember to check whether the user has supplied us with a good
	// address!
	struct Env *env;
	int ret = envid2env(envid, &env, true);
	if (ret < 0)
		return ret;

	user_mem_assert(curenv, tf, sizeof(*tf), PTE_U);

	tf->tf_cs = curenv->env_tf.tf_cs;
	tf->tf_ds = curenv->env_tf.tf_ds;
	tf->tf_eflags &= ~FL_IOPL_MASK;
	tf->tf_eflags |= FL_IF;

	env->env_tf = *tf;
	return 0;
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.
	struct Env *env;
	int ret = envid2env(envid, &env, true);
	if (ret < 0)
		return ret;
	env->env_pgfault_upcall = func;
	return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!


	if ((uintptr_t) va >= UTOP || (uintptr_t) va % PGSIZE != 0)
		return -E_INVAL;

	// Make sure perm consist only of flags in PTE_SYSCALl
	// and make sure U,P are set
	if ((perm & ~PTE_SYSCALL) || !(perm & (PTE_U | PTE_P)))
	{
		cprintf("sys_page_alloc bad permissions \n");
		return -E_INVAL;
	}

	struct PageInfo *page_info = page_alloc(ALLOC_ZERO);
	if (page_info == NULL)
		return -E_NO_MEM;

	struct Env *env;
	int ret = envid2env(envid, &env, true);
	if (ret < 0)
		return ret;
	ret = page_insert(env->env_pgdir, page_info, va, perm);
	if (ret < 0)
		return ret;
	return 0;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
			 envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.


	// Get env structures for each envid
//	cprintf("sys_page_map: srcenvid=%d, dstenvid=%d, srcva=%p, dstva=%p, perm=%d\n", srcenvid, dstenvid, srcva, dstva,
//			perm);

	struct Env *src_env, *dst_env;
	int ret = envid2env(srcenvid, &src_env, true);
	if (ret < 0)
		return ret;
	ret = envid2env(dstenvid, &dst_env, true);
	if (ret < 0)
		return ret;

	// Make sure that addresses are valid
	if ((uintptr_t) srcva >= UTOP || (uintptr_t) srcva % PGSIZE != 0 ||
		(uintptr_t) dstva >= UTOP || (uintptr_t) dstva % PGSIZE != 0)
		return -E_INVAL;

	// Make sure perm consist only of flags in PTE_SYSCALl
	// and make sure U,P are set
	if ((perm & ~PTE_SYSCALL) || !(perm & (PTE_U | PTE_P)))
		return -E_INVAL;

	pte_t *pte_entry;
	struct PageInfo *page_info = page_lookup(src_env->env_pgdir, srcva, &pte_entry);
	// Page is not mapped
	if (page_info == NULL)
		return -E_INVAL;

	// The user requested write permissions when the page is read only
	if ((perm & PTE_W) && !(*pte_entry & PTE_W))
		return -E_INVAL;

	ret = page_insert(dst_env->env_pgdir, page_info, dstva, perm);
	if (ret < 0)
		return ret;
	return 0;
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	struct Env *env;
	int ret = envid2env(envid, &env, true);
	if (ret < 0)
		return ret;
	if ((uint32_t) va >= UTOP || (uint32_t) va % PGSIZE != 0)
		return -E_INVAL;
	page_remove(env->env_pgdir, va);
	return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.
	struct Env *dstenv;
	// Check that the env exists
	int ret = envid2env(envid, &dstenv, false);
	if (ret < 0)
		return ret;

	// If target env is not blocking right now
	if (!dstenv->env_ipc_recving)
		return -E_IPC_NOT_RECV;
//	if (dstenv->env_ipc_from != 0 && dstenv->env_ipc_from != curenv->env_id)
//		return -E_IPC_NOT_RECV;

	// This is asserted because env_ipc_receiving is true
//	assert(dstenv->env_status == ENV_NOT_RUNNABLE);


	// If caller sent a page
	pte_t *srcva_pte;
	if ((size_t) srcva < UTOP)
	{
		// Check page is aligned
		if ((size_t) srcva % PGSIZE != 0)
			return -E_INVAL;
		// Inappropriate permissions
		if ((perm & ~PTE_SYSCALL) || !(perm & (PTE_U | PTE_P)))
			return -E_INVAL;

		// Srcva not mapped
		if (page_lookup(curenv->env_pgdir, srcva, &srcva_pte) == NULL)
			return -E_INVAL;

		// Write permission, but src address is not writable
		if ((perm & PTE_W) && !(*srcva_pte & PTE_W))
			return -E_INVAL;

		// Send page
		// Send page mapping if dest environment is recieving one
		if ((uintptr_t) (dstenv->env_ipc_dstva) < UTOP)
		{
			ret = page_insert(dstenv->env_pgdir, pa2page(PTE_ADDR(*srcva_pte)), dstenv->env_ipc_dstva, perm);
			if (ret < 0)
				return ret;
			dstenv->env_ipc_perm = perm;
		}
	} else
	{
		dstenv->env_ipc_perm = 0;
	}

	dstenv->env_ipc_recving = false;
	dstenv->env_ipc_from = curenv->env_id;
	dstenv->env_ipc_value = value;

	dstenv->env_tf.tf_regs.reg_eax = 0; // target env syscall returns with value 0
	dstenv->env_status = ENV_RUNNABLE;
	return 0;
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	if ((size_t) dstva < UTOP && (size_t) dstva % PGSIZE != 0)
		return -E_INVAL;
	// Block until a value os ready
	curenv->env_status = ENV_NOT_RUNNABLE;
	curenv->env_ipc_recving = true;

	if ((size_t) dstva < UTOP)
		curenv->env_ipc_dstva = dstva;
//	cprintf("Sched yeilding!\n");
	sched_yield();
}

// Return the current time.
static int
sys_time_msec(void)
{
	// LAB 6: Your code here.
	return time_msec();
}

// try transmit a packet
static int sys_try_transmit_packet(const uint8_t *packet_data, uint32_t packet_size)
{
	user_mem_assert(curenv, packet_data, packet_size, PTE_P | PTE_U);
	return e1000_try_transmit_packet(packet_data, packet_size);
}

// try recv a packet
static int sys_try_recv_packet(uint8_t *buffer, uint32_t buffer_size, uint32_t *packet_size)
{
	user_mem_assert(curenv, buffer, buffer_size, PTE_P | PTE_U | PTE_W);
	user_mem_assert(curenv, packet_size, sizeof(uint32_t), PTE_P | PTE_U | PTE_W);

	return e1000_try_recv_packet(buffer, buffer_size, packet_size);
}


// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.
	uint32_t retvalue = 0;

//	cprintf("syscall: no: %u, a1=0x%08x, a2=0x%08x, a3=0x%08x, a4=0x%08x, a5=0x%08x\r\n", syscallno, a1, a2, a3, a4,
//			a5);

	switch (syscallno)
	{
		case SYS_cputs:
			sys_cputs((const char *) a1, (size_t) a2);
			break;
		case SYS_cgetc:
			retvalue = (uint32_t) sys_cgetc();
			break;
		case SYS_getenvid:
			retvalue = (uint32_t) sys_getenvid();
			break;
		case SYS_env_destroy:
			retvalue = (uint32_t) sys_env_destroy((envid_t) a1);
			break;
		case SYS_page_alloc:
			retvalue = (uint32_t) sys_page_alloc((envid_t) a1, (void *) a2, (int) a3);
			break;
		case SYS_page_map:
			retvalue = (uint32_t) sys_page_map((envid_t) a1, (void *) a2, (envid_t) a3, (void *) a4, (int) a5);
			break;
		case SYS_page_unmap:
			retvalue = (uint32_t) sys_page_unmap((envid_t) a1, (void *) a2);
			break;
		case SYS_exofork:
			retvalue = (uint32_t) sys_exofork();
			break;
		case SYS_env_set_status:
			retvalue = (uint32_t) sys_env_set_status((envid_t) a1, (int) a2);
			break;
		case SYS_env_set_pgfault_upcall:
			retvalue = (uint32_t) sys_env_set_pgfault_upcall((envid_t) a1, (void *) a2);
			break;
		case SYS_yield:
			sys_yield();
			break;
		case SYS_env_set_trapframe:
			retvalue = sys_env_set_trapframe(a1, (struct Trapframe *) a2);
			break;
		case SYS_ipc_try_send:
			retvalue = (uint32_t) sys_ipc_try_send((envid_t) a1, (uint32_t) a2, (void *) a3, (unsigned int) a4);
			break;
		case SYS_ipc_recv:
			retvalue = (uint32_t) sys_ipc_recv((void *) a1);
			break;
		case SYS_time_msec:
			retvalue = sys_time_msec();
			break;
		case SYS_try_transmit_packet:
			retvalue = sys_try_transmit_packet((const uint8_t *) a1, a2);
			break;
		case SYS_try_recv_packet:
			retvalue = sys_try_recv_packet((uint8_t *) a1, a2, (uint32_t *) a3);
			break;

		default:
			return -E_INVAL;
	}

	return retvalue;
}