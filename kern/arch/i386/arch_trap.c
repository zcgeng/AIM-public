/*************************************************************************
	> File Name: arch_trap.c
	> Author: Zhao Changgeng
	> Mail: zcg1996@gmail.com
	> Created Time: 2016年11月01日 星期二 22时48分54秒
 ************************************************************************/
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
#include <aim/trap.h>
#define INTERRUPT_GATE_32   0xE
#define TRAP_GATE_32        0xF
#define NR_IRQ              256

/* Each entry of the IDT is either an interrupt gate, or a trap gate */ // i386 specific
static struct gatedesc idt[NR_IRQ];

/* Setup a interrupt gate for interrupt handler. */ // i386 specific
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

/* Setup a trap gate for cpu exception. */ // i386 specific
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


// belows are defined in do_irq.S and are obviously i386 specific
void irq0();
void irq1();
void irq14();
void irq38();
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

void idt_init(){ // i386 specific
	int i;
	for (i = 0; i < NR_IRQ; i ++) {
		set_trap(idt + i, SEG_KCODE << 3, (uint32_t)irq_empty, DPL_KERNEL);
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
	set_intr(idt+32 + 38, SEG_KCODE << 3, (uint32_t)irq38, DPL_KERNEL);

	/* the ``idt'' is its virtual address */
	write_idtr(idt, sizeof(idt));
}

void irq_handle(struct trapframe *tf) {
	int irq = tf->trapno;

	if (irq < 0) {
		/* "irq_empty" pushed -1 in the trapframe*/
		panic("Unhandled exception!\n");
	} else if (irq == 0x80) {
		tf->eax = handle_syscall(tf->eax, tf->ebx, tf->ecx, tf->edx, tf->esi, tf->edi, tf->ebp); // i386 specific
	} else{
		handle_interrupt(irq);
	}
}
