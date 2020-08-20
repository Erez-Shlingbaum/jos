/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

#define _TRAPNAME(num) trap_ ## num
#define TRAPNAME(num) _TRAPNAME(num)

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

#define MYTRAPHANDLER(num) TRAPHANDLER(TRAPNAME(num), num)

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

#define MYTRAPHANDLER_NOEC(num) TRAPHANDLER_NOEC(TRAPNAME(num), num)

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
MYTRAPHANDLER_NOEC(T_DIVIDE);	// 0
MYTRAPHANDLER_NOEC(T_DEBUG);	// 1
MYTRAPHANDLER_NOEC(T_NMI);		// 2
MYTRAPHANDLER_NOEC(T_BRKPT);	// 3
MYTRAPHANDLER_NOEC(T_OFLOW);	// 4
MYTRAPHANDLER_NOEC(T_BOUND);	// 5
MYTRAPHANDLER_NOEC(T_ILLOP);	// 6
MYTRAPHANDLER_NOEC(T_DEVICE);	// 7
MYTRAPHANDLER(T_DBLFLT);		// 8
								// 9
MYTRAPHANDLER(T_TSS);			// 10
MYTRAPHANDLER(T_SEGNP);			// 11
MYTRAPHANDLER(T_STACK);			// 12
MYTRAPHANDLER(T_GPFLT);			// 13
MYTRAPHANDLER(T_PGFLT);			// 14
								// 15
MYTRAPHANDLER_NOEC(T_FPERR);	// 16
MYTRAPHANDLER(T_ALIGN);			// 17
MYTRAPHANDLER_NOEC(T_MCHK);		// 18
MYTRAPHANDLER_NOEC(T_SIMDERR);	// 19
								// 20
								// 21
								// 22
								// 23
								// 24
								// 25
								// 26
								// 27
								// 28
								// 29
								// 30 - Security Exception (push errorcode)
								//

MYTRAPHANDLER_NOEC(T_SYSCALL);		// 48


_alltraps:
	pushl %ds
	pushl %es
	pushal
	movw $GD_KD, %ax
	movw %ax, %ds
	movw %ax, %es
	pushl %esp
	call trap


// Return from _alltraps is at env_pop_tf
