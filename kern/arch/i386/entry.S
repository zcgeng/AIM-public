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
#include <aim/memlayout.h>
#include <arch/i386/mmu.h>

.text

# By convention, the _start symbol specifies the ELF entry point.
# Since we haven't set up virtual memory yet, our entry point is
# the physical address of 'entry'.
.globl _start
_start = V2P_WO(master_early_entry)

.globl master_early_entry
master_early_entry:
	cli

	# clean .bss
	cld
	mov $_bss_start, %eax
	mov $_bss_end, %ecx
	sub $_bss_start, %ecx
	rep stosb
	
	# Set up the stack pointer.
	mov	$(V2P_WO(kstack_top)), %esp
	xor	%eax, %eax
	push	%eax		# eip=0
	push	%eax		# ebp=0
	mov	%esp, %ebp
	
	call master_early_init

.globl turn_on_mmu
turn_on_mmu:
	# Turn on page size extension for 4Mbyte pages
	movl    %cr4, %eax
	orl     $(CR4_PSE), %eax
	movl    %eax, %cr4
	# Set page directory 
	movl    $(V2P_WO(entrypgdir)), %eax;  # defined in arch_early_init.c
	movl    %eax, %cr3
	# Turn on paging
	movl    %cr0, %eax
	orl     $(CR0_PG), %eax
	orl	$(CR0_WP), %eax
	movl    %eax, %cr0
	ret
