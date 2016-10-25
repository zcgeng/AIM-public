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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <aim/init.h>
#include <aim/panic.h>
#include <aim/mmu.h>
#include <aim/memlayout.h>
#include <arch-mmu.h>
#include <asm.h>

// kernel gdt
struct segdesc gdt[] = {
	SEG(0x0, 0x0, 0x0, 0x0),			// null seg
	SEG(STA_X|STA_R, 0, 0xffffffff, 0),		// kernel code
	SEG(STA_W, 0, 0xffffffff, 0),			// kernel data
	SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER),	// user code
	SEG(STA_W, 0, 0xffffffff, DPL_USER)		// user data
};

void early_mm_init();
void high_address_entry();
extern pgindex_t entrypgdir[];
extern uint32_t kstack_top;
__noreturn
void arch_early_init(void)
{
	lgdt(gdt, sizeof(gdt)); // setup kernel segment descriptors.
	early_mm_init();
	/* Turn on page size extension for 4Mbyte pages */
	asm(
		"mov    %%cr4, %%eax;"
		"or     %0, %%eax;"
		"mov    %%eax, %%cr4;"
		::"i"(CR4_PSE)
	);
	/* Set page directory */
	asm(
		"mov    %0, %%cr3;"
		::"r"(V2P(entrypgdir))
	);
	/* Turn on paging */
	asm(
		"mov	%%cr0, %%eax;"
		"or	%0, %%eax;"
		"mov	%%eax, %%cr0;"
		::"i"(CR0_PG | CR0_WP)
	);
	/* Set up the stack pointer. */
	asm(
		"mov	%0, %%esp;"
		"mov	%%esp, %%ebp"
		::"r"(V2P(&kstack_top))
	);
	
	/* jump to some high address! */
	abs_jump(high_address_entry);
}

