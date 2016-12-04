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
#include <arch-trap.h>
#include <aim/smp.h>

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
	extern void lapiceoi();

	switch(irq){
		case T_IRQ0 + IRQ_TIMER:
			kpdebug("CPU%d received a timer interrupt\n", cpuid());
			lapiceoi();
			//yield();
			break;
		case T_IRQ0 + PANIC_INTERRUPT_NUM :
			__local_panic(); // IPI panic
			break;
		default:
			kpdebug("<IRQ %d>\n", irq);
			lapiceoi();
			break;
	}
}

void trap_init(void){
	/* forbid the outside interruptions
	 * (but how to "forbid it one by one"?) */
	cli();

	/* interrupt table preparation */
	idt_init();
}
