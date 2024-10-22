// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include "pmap.h"
#include "env.h"

#define CMDBUF_SIZE    80    // enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char **argv, struct Trapframe *tf);
};

static struct Command commands[] = {
		{"help",      "Display this list of commands",        mon_help},
		{"kerninfo",  "Display information about the kernel", mon_kerninfo},
		{"backtrace", "Print backtrace",                      mon_backtrace},
		{"shutdown",  "QEMU SPECIFIC SHUTDOWN",               mon_qemu_shutdown},
		{"ppm",       "print page mappings",                  mon_print_page_mappings},
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
			ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	struct frame {
		void *ebp;
		void *eip;
		void *args[5];
	};

	struct frame *current_frame = (struct frame *) read_ebp();

	cprintf("Stack backtrace:\n");
	do
	{
		// Print frame info
		cprintf("  ebp %08x  eip %08x  args", current_frame, current_frame->eip);
		for (int i = 0; i < sizeof(current_frame->args) / sizeof(*current_frame->args); i++)
			cprintf(" %08x", current_frame->args[i]);
		cprintf("\n");

		struct Eipdebuginfo eipdebuginfo = {0};
		// Print debug info
		debuginfo_eip((uintptr_t) current_frame->eip, &eipdebuginfo);
		cprintf(
				"%s:%d: %.*s+%d\n",
				eipdebuginfo.eip_file,
				eipdebuginfo.eip_line,
				eipdebuginfo.eip_fn_namelen,
				eipdebuginfo.eip_fn_name,
				(uintptr_t) current_frame->eip - (uintptr_t) eipdebuginfo.eip_fn_addr
		);

		current_frame = (struct frame *) current_frame->ebp;
	} while (current_frame != NULL);

	return 0;
}

int
mon_qemu_shutdown(int argc, char **argv, struct Trapframe *tf)
{
	outw(0x604, 0x2000);
	return 0;
}

int
mon_print_page_mappings(int argc, char **argv, struct Trapframe *tf)
{
	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1)
	{
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS - 1)
		{
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++)
	{
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1)
	{
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
