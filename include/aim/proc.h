/* Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
 *
 * This file is part of AIM.
 *
 * AIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PROC_H
#define _PROC_H

#include <aim/limits.h>
#include <aim/namespace.h>
#include <aim/uvm.h>
#include <context.h>
//#include <list.h>
//#include <file.h>
#include <arch-mmu.h>

// Per-CPU state
#define NCPU 8
#define NOFILE       16  // open files per process
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?

  // Cpu-local storage variables; see below
  struct cpu *cpu;
  struct proc *proc;           // The currently-running process.
};

extern struct cpu cpus[NCPU];
extern int ncpu;

// Per-CPU variables, holding pointers to the
// current cpu and to the current process.
// The asm suffix tells gcc to use "%gs:0" to refer to cpu
// and "%gs:4" to refer to proc.  seginit sets up the
// %gs segment register so that %gs refers to the memory
// holding those two variables in the local cpu's struct cpu.
// This is similar to how thread-local variables are implemented
// in thread libraries such as Linux pthreads.
// extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
// extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc
void set_gs_cpu(struct cpu* cpu);
struct cpu* get_gs_cpu();
void set_gs_proc(struct proc* proc);
struct proc* get_gs_proc();

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc_xv6 {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
};

struct proc {
	/* TODO: move thread-specific data into a separate structure */
	/*
	 * the kernel stack pointer is used to prepare C runtime, thus accessed
	 * in assembly. Force it here at offset 0 for easy access.
	 */
	void *kstack; /* bottom, or lower address */
	size_t kstack_size;

	/* other stuff go here */
	int		tid;	/* Thread ID (unused - for multithreading) */
	pid_t		pid;	/* Process ID within namespace @namespace */
	pid_t		kpid;	/* Process ID */
	unsigned int	state;	/* Process state (runnability) */
	/* The state values come from OpenBSD */
	/* TODO: may have more...? */
#define PS_EMBRYO	1	/* Just created */
#define PS_RUNNABLE	2	/* Runnable */
#define PS_SLEEPING	3	/* Currently sleeping */
#define PS_ZOMBIE	4
#define	PS_ONPROC	5
	unsigned int	exit_code;	/* Exit code */
	unsigned int	exit_signal;	/* Signal code */
	uint32_t	flags;	/* Flags */
	/* TODO: may have more...? */
#define PF_EXITING	0x00000004	/* getting shut down */
#define PF_SIGNALED	0x00000400	/* killed by a signal */
#define PF_KTHREAD	0x00200000	/* I am a kernel thread */
	int		oncpu;		/* CPU ID being running on */
#define CPU_NONE	-1
	uintptr_t	bed;		/* object we are sleeping on */
	struct namespace *namespace;	/* Namespace */
	struct mm 	*mm; /* Memory mapping structure including pgindex */
	/*
	 * Expandable heap is placed directly above user stack.
	 * User stack is placed above all loaded program segments.
	 * We put program arguments above user stack.
	 */
	struct context	context;	/* Context before switch */
	size_t		heapsize;	/* Expandable heap size */
	void		*heapbase;	/* Expandable heap base */

	char		name[PROC_NAME_LEN_MAX];

	/* File system related */
	//struct vnode	*cwd;		/* current working directory */
	//struct vnode	*rootd;		/* root directory (chroot) */
	//union {
	//	struct {
	//		struct file *fstdin;	/* stdin */
	//		struct file *fstdout;	/* stdout */
	//		struct file *fstderr;	/* stderr */
	//	};
	//	struct file *fd[OPEN_MAX];	/* opened files */
	//};
	//lock_t		fdlock;		/* lock of file table */

	/* TODO: POSIX process groups and sessions.  I'm not entirely sure
	 * how they should look like. */
	struct proc	*groupleader;	/* Process group leader */
	struct proc	*sessionleader;	/* Session leader */

	/* Session data */
	struct tty_device *tty;		/* Controlling terminal */
	//struct vnode	*ttyvnode;	/* vnode of terminal */

	/* Process tree related */
	struct proc	*mainthread;	/* Main thread of the same process */
	struct proc	*parent;
	struct proc	*first_child;
	struct proc	*next_sibling;
	struct proc	*prev_sibling;

	//struct scheduler *scheduler;	/* Scheduler for this process */
	//struct list_head sched_node;	/* List node in scheduler */
};

static inline void *kstacktop(struct proc *proc)
{
	return proc->kstack + proc->kstack_size;
}

/* Create a struct proc inside namespace @ns and initialize everything if we
 * can by default. */
struct proc *proc_new(struct namespace *ns);
/* Exact opposite of proc_new */
void proc_destroy(struct proc *proc);
void proc_init(void);
/* Setup per-CPU idle process */
void idle_init(void);
void spawn_initproc(void);
pid_t pid_new(pid_t kpid, struct namespace *ns);
void pid_recycle(pid_t pid, struct namespace *ns);

/* The following are architecture-specific code */

/*
 * Setup a kernel process with entry and arguments.
 * A kernel process always works on its kernel stack and with default
 * kernel memory mapping.
 *
 * Only sets up process trap frame and context.
 *
 * For architecture developers: you are not required to implement
 * proc_ksetup().  You only need to provide arch-dependent code
 * __proc_ksetup() (see kern/proc/proc.c)
 */
void proc_ksetup(struct proc *proc, void *entry, void *args);
/*
 * Setup a user process with entry, stack top, and arguments.
 *
 * The user-mode counterpart of proc_ksetup().
 *
 * For architecture developers: You only need to provide arch-dependent
 * code __proc_usetup().
 */
void proc_usetup(struct proc *proc, void *entry, void *stacktop, void *args);
void switch_context(struct proc *proc);
/* Return to trap frame in @proc.  Usually called once by fork child */
void proc_trap_return(struct proc *proc);

/*
 * Process tree maintenance
 */
void proctree_add_child(struct proc *child, struct proc *parent);


// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

#endif /* _PROC_H */
