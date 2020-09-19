// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW        0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).
	// LAB 4: Your code here.

	pde_t pde_addr = uvpd[PDX(addr)];
	pte_t pte_addr = uvpt[PGNUM(addr)];

	// if not write or not cow
	if ((err & FEC_WR) == 0 ||
		(pde_addr & PTE_P) == 0 ||
		(~pte_addr & (PTE_P | PTE_COW)) != 0)
		panic("pgfault: not write or not COW\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.
	// LAB 4: Your code here.
	addr = (void *) ROUNDDOWN(addr, PGSIZE);

	int ret = sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W);
	if (ret < 0)
		panic("sys page alloc: %e\n", ret);

	memcpy(PFTEMP, addr, PGSIZE);
	ret = sys_page_map(0, PFTEMP, 0, addr, PTE_P | PTE_U | PTE_W);
	if (ret < 0)
		panic("sys page map: %e\n", ret);

	ret = sys_page_unmap(0, PFTEMP);
	if (ret < 0)
		panic("sys page unmap: %e\n", ret);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	// LAB 4: Your code here.

	pte_t pte_addr = uvpt[pn];
	// I assume this function is called with good pages
	if ((pte_addr & PTE_P) == 0)
		panic("PTE NOT PRESENT\n");

	void *addr = (void *) (pn * PGSIZE);

	// If page is W or COW
	if ((pte_addr & (PTE_W | PTE_COW)) != 0)
	{
		int perm = PTE_P | PTE_U | PTE_COW;

		// Map child
		int ret = sys_page_map(0, addr, envid, addr, perm);
		if (ret < 0)
		{
			panic("duppage: child mapping, sys_page_map - %e", ret);
			return ret;
		}
		// Remap parent
		ret = sys_page_map(0, addr, 0, addr, perm);
		if (ret < 0)
		{
			panic("duppage: parent remapping, sys_page_map - %e", ret);
			return ret;
		}
	} else
	{
		int perm = PTE_P | PTE_U;

		// Map child
		int ret = sys_page_map(0, addr, envid, addr, perm);
		if (ret < 0)
		{
			cprintf("addr=%p", addr);
			panic("duppage: child mapping, sys_page_map - %e", ret);
			return ret;
		}
	}
	return 0;
}

extern void _pgfault_upcall(void);

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// This will be set for both father and child
	set_pgfault_handler(pgfault);

	// Fork!
	envid_t child_envid = sys_exofork();
	if (child_envid < 0)
		return child_envid;

	// Child
	if (child_envid == 0)
	{
		thisenv = &envs[ENVX(sys_getenvid())];
		return child_envid;
	}
	/* father: */

	// Set upcall for child
	sys_env_set_pgfault_upcall(child_envid, _pgfault_upcall);
	// Allocate new exception stack for child
	sys_page_alloc(child_envid, (void *) (UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W);

	unsigned int num_pages = PGNUM(UTOP);
	for (size_t i = 0; i < num_pages; ++i)
	{
		uintptr_t addr = i * PGSIZE;
		if (addr == UXSTACKTOP - PGSIZE)
			continue;

		pde_t pde_addr = uvpd[PDX(addr)];

		if ((pde_addr & PTE_P) == 0)
			continue;

		pte_t pte_addr = uvpt[i];
		if ((pte_addr & PTE_P) == 0)
			continue;

		int ret = duppage(child_envid, i);
		if (ret < 0)
		{
			panic("FUCK %d\n", ret);
//			return ret;
		}
	}
	sys_env_set_status(child_envid, ENV_RUNNABLE);
	return child_envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
