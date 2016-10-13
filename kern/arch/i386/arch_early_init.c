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
#include <arch-mmu.h>
#include <arch/i386/x86.h>

// kernel gdt
struct segdesc gdt[] = {
	SEG(0x0, 0x0, 0x0, 0x0),			// null seg
	SEG(STA_X|STA_R, 0, 0xffffffff, 0),		// kernel code
	SEG(STA_W, 0, 0xffffffff, 0),			// kernel data
	SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER),	// user code
	SEG(STA_W, 0, 0xffffffff, DPL_USER)		// user data
};

__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // Map VA's [0, 4MB) to PA's [0, 4MB)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  [KERN_BASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};

__noreturn void turn_on_mmu();
__noreturn
void arch_early_init(void)
{
	lgdt(gdt, sizeof(gdt)); // setup kernel segment descriptors.
	turn_on_mmu(); // defined in entry.S
}

