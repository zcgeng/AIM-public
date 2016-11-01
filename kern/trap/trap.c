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

void irq0();
void irq1();
void irq14();
void vec0();
void vec1();
void vec2();
void vec3();
void vec4();
void vec5();
void vec6();
void vec7();
void vec8();
void vec9();
void vec10();
void vec11();
void vec12();
void vec13();
void vec14();
void vecsys();

void irq_empty();

static void idt_init(){
	int i;
	for (i = 0; i < NR_IRQ; i ++) {
		set_trap(idt + i, SEG_KCODE << 3, (uint32_t)handle_syscall, DPL_KERNEL);
	}
	set_trap(idt + 0, SEG_KCODE << 3, (uint32_t)vec0, DPL_KERNEL);
	set_trap(idt + 1, SEG_KCODE << 3, (uint32_t)vec1, DPL_KERNEL);
	set_trap(idt + 2, SEG_KCODE << 3, (uint32_t)vec2, DPL_KERNEL);
	set_trap(idt + 3, SEG_KCODE << 3, (uint32_t)vec3, DPL_KERNEL);
	set_trap(idt + 4, SEG_KCODE << 3, (uint32_t)vec4, DPL_KERNEL);
	set_trap(idt + 5, SEG_KCODE << 3, (uint32_t)vec5, DPL_KERNEL);
	set_trap(idt + 6, SEG_KCODE << 3, (uint32_t)vec6, DPL_KERNEL);
	set_trap(idt + 7, SEG_KCODE << 3, (uint32_t)vec7, DPL_KERNEL);
	set_trap(idt + 8, SEG_KCODE << 3, (uint32_t)vec8, DPL_KERNEL);
	set_trap(idt + 9, SEG_KCODE << 3, (uint32_t)vec9, DPL_KERNEL);
	set_trap(idt + 10, SEG_KCODE << 3, (uint32_t)vec10, DPL_KERNEL);
	set_trap(idt + 11, SEG_KCODE << 3, (uint32_t)vec11, DPL_KERNEL);
	set_trap(idt + 12, SEG_KCODE << 3, (uint32_t)vec12, DPL_KERNEL);
	set_trap(idt + 13, SEG_KCODE << 3, (uint32_t)vec13, DPL_KERNEL);
	set_trap(idt + 14, SEG_KCODE << 3, (uint32_t)vec14, DPL_KERNEL);

	/* the system call 0x80 */
	set_trap(idt + 0x80, SEG_KCODE << 3, (uint32_t)vecsys, DPL_USER);

	set_intr(idt+32 + 0, SEG_KCODE << 3, (uint32_t)irq0, DPL_KERNEL);
	set_intr(idt+32 + 1, SEG_KCODE << 3, (uint32_t)irq1, DPL_KERNEL);
	set_intr(idt+32 + 14, SEG_KCODE << 3, (uint32_t)irq14, DPL_KERNEL);

	/* the ``idt'' is its virtual address */
	write_idtr(idt, sizeof(idt));
}
void trap_init(void){

	// interrupt table preparation
	idt_init();

	// LAPIC & IOPAC
    
	// ban the outside interruptions
}
