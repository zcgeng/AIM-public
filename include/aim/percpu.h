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

#ifndef _PERCPU_H
#define _PERCPU_H

#include <sys/types.h>

#include <arch-mmu.h>
#include <aim/smp.h>	/* cpuid(), arch directory */

// Per-CPU state
#define NCPU 8

struct percpu {
	/*
	 * to retrieve the kernel stack, this pointer need to be accessed from
	 * within the assembly code. Keep it here as the first element.
	 */
	struct proc *proc;

	/* other stuff go here */
	uchar apicid;                // Local APIC ID
	struct context *scheduler;   // swtch() here to enter scheduler
	struct taskstate ts;         // Used by x86 to find stack for interrupt
	struct segdesc gdt[NSEGS];   // x86 global descriptor table
	volatile uint started;       // Has the CPU started?
	int ncli;                    // Depth of pushcli nesting.
	int intena;                  // Were interrupts enabled before pushcli?

	// Cpu-local storage variables; see below
	struct percpu *percpu;
};

extern struct percpu cpus[];
/* Idle proc is per-cpu dummy process here. */
//extern struct proc idleproc[];

#define cpu		cpus[cpuid()]
#define current_proc	cpu.proc
#define cpu_idleproc	(&idleproc[cpuid()])

#endif /* _PERCPU_H */
