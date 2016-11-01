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
#include <aim/console.h>
#include <libc/stdarg.h>
#include <libc/stddef.h>
#include <libc/stdio.h>
#include <arch-mmu.h>
#include <asm.h>
#include <aim/panic.h>

long handle_syscall(long number, ...)
{
	va_list ap;

	va_start(ap, number);
	kpdebug("<SYSCALL %ld>\n", number);
	for (int i = 0; i < 6; i += 1) {
		ulong arg = va_arg(ap, ulong);
		/* always evaluated at compile time thus optimized out */
		if (sizeof(long) == sizeof(int)) {
			/* 32 bit */
			kpdebug("arg %d = 0x%08x\n", i, arg);
		} else {
			/* 64 bit */
			kpdebug("arg %d = 0x%016x\n", i, arg);
		}
	}
	va_end(ap);

	return 0;
}

void handle_interrupt(int irq)
{
	kpdebug("<IRQ %d>\n", irq);
}

void trap_panic(int num){
	panic("idt number not valid: %d\n", num);
}

#define INTERRUPT_GATE_32   0xE
#define TRAP_GATE_32        0xF
#define NR_IRQ              256

/* Each entry of the IDT is either an interrupt gate, or a trap gate */
static struct gatedesc idt[NR_IRQ];

/* Setup a interrupt gate for interrupt handler. */
static void set_intr(struct gatedesc *ptr, uint32_t selector, uint32_t offset, uint32_t dpl) {
	ptr->off_15_0 = offset & 0xFFFF;
	ptr->cs = selector;
	ptr->args = 0;
	ptr->type = INTERRUPT_GATE_32;
	ptr->s = 0;
	ptr->dpl = dpl;
	ptr->p = 1;
	ptr->off_31_16 = (offset >> 16) & 0xFFFF;
}

/* Setup a trap gate for cpu exception. */
static void set_trap(struct gatedesc *ptr, uint32_t selector, uint32_t offset, uint32_t dpl) {
	ptr->off_15_0 = offset & 0xFFFF;
	ptr->cs = selector;
	ptr->args = 0;
	ptr->type = TRAP_GATE_32;
	ptr->s = 0;
	ptr->dpl = dpl;
	ptr->p = 1;
	ptr->off_31_16 = (offset >> 16) & 0xFFFF;
}

static void idt_init(){
	int i;
	for (i = 0; i < NR_IRQ; i ++) {
		set_trap(idt + i, SEG_KCODE << 3, (uint32_t)trap_panic, DPL_KERNEL);
	}
	/* the system call 0x80 */
	set_trap(idt + 0x80, SEG_KERNEL_CODE << 3, (uint32_t)handle_syscall, DPL_USER);
	set_intr(idt+32 + 0, SEG_KCODE << 3, (uint32_t)handle_interrupt, DPL_KERNEL);

	/* the ``idt'' is its virtual address */
	write_idtr(idt, sizeof(idt));
}

void trap_init(void){

	// interrupt table preparation
	idt_init();

	// LAPIC & IOAPAC
	
	// forbid the outside interruptions
}
