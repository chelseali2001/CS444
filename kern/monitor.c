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

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

// LAB 1: add your command to here...
static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
  { "backtrace", "Show the backtrace of the current kernel stack", mon_backtrace },
  { "show", "Prints a beautiful ASCII Art with 5 or more colors", mon_show }
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

// This function prints the stack backtrace
struct Eipdebuginfo 
print_backtrace(int *ebp, struct Eipdebuginfo eipInfo) 
{
  cprintf("  ebp %08x eip %08x args ", ebp, ebp[1]); // Printing the ebp (base pointer) and eip (return instruction pointer)
  
  // Printing the first five arguments to the function in question
  for (int x = 2; x <= 6; x++)
    cprintf("%08x ", ebp[x]);
  
  cprintf("\n");
  
  debuginfo_eip(ebp[1], &eipInfo); //looks up eip in the symbol table and returns the debugging information for that address
  int eip = (eipInfo.eip_fn_addr - ebp[1]) * -1;
  
  // Prints the file name and line within that file of the stack frame’s eip, followed by the name of the function and the offset of the eip from the first instruction of the function
  cprintf("    %s:%d: %.*s+%d\n", eipInfo.eip_file, eipInfo.eip_line, eipInfo.eip_fn_namelen, eipInfo.eip_fn_name, eip);
  
  return eipInfo;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// LAB 1: Your code here.
    // HINT 1: use read_ebp().
    // HINT 2: print the current ebp on the first line (not current_ebp[0])
  
  cprintf("Stack backtrace:\n");
  
  int *ebp = (int *) read_ebp();
  struct Eipdebuginfo eipInfo;
  int eip = 0;
  
  // Printing all of the stack frames
  for (;ebp[0] != 0; ebp += 8) 
    eipInfo = print_backtrace(ebp, eipInfo);

  eipInfo = print_backtrace(ebp, eipInfo); // Printing last stack frame 
  
	return 0;
}

int
mon_show(int argc, char **argv, struct Trapframe *tf) 
{
  cprintf("\x1b[34mA simple smile :)\n");
  cprintf("\x1b[31m  0   0\n");
  cprintf("\x1b[33m    !\n");
  cprintf("\x1b[32m1       1\n");
  cprintf("\x1b[36m1       1\n");
  cprintf("\x1b[35m -------\n");
  cprintf("\x1b[0m1st row is red, 2nd row is yellow, 3rd row is green, 4th row is cyan, 5th row is purple");
  
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
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
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
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
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

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
