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
#include <aim/device.h>
#include <aim/early_kmmap.h>
#include <aim/init.h>
#include <aim/mmu.h>
#include <aim/panic.h>
#include <aim/pmm.h>
#include <aim/vmm.h>
#include <drivers/io/io-mem.h>
#include <drivers/io/io-port.h>
#include <platform.h>
#include <aim/trap.h>
#include <aim/initcalls.h>
#include <asm.h>
#include <aim/proc.h>
#include <aim/sync.h>
#include <aim/memlayout.h>
#include <aim/smp.h>

static inline
int early_devices_init(void)
{
#ifdef IO_MEM_ROOT
	if (io_mem_init(&early_memory_bus) < 0)
		return EOF;
#endif /* IO_MEM_ROOT */

#ifdef IO_PORT_ROOT
	if (io_port_init(&early_port_bus) < 0)
		return EOF;
#endif /* IO_PORT_ROOT */
	return 0;
}

void arch_early_init();
extern uint32_t _bss_start;
extern uint32_t _bss_end;

static void clear_bss(){
	void* start = (void*)&_bss_start;
	void* end = (void*)&_bss_end;
	int count = ((uint32_t)end - (uint32_t)start) / 4;
	stosb(start , 0, count);
}

__noreturn
void master_early_init(void)
{
	clear_bss();
	/* clear address-space-related callback handlers */
	early_mapping_clear();
	mmu_handlers_clear();
	/* prepare early devices like memory bus and port bus */
	if (early_devices_init() < 0)
		goto panic;
	/* other preperations, including early secondary buses */

	if (early_console_init(
		EARLY_CONSOLE_BUS,
		EARLY_CONSOLE_BASE,
		EARLY_CONSOLE_MAPPING
	) < 0)
		panic("Early console init failed.\n");
	kputs("Hello, world!\n");
	do_early_initcalls();
	arch_early_init();

panic:
	panic("panic in early_init.c !");
}



void allocator_init(){
	struct page_allocator a = {
		page_alloc, page_free, page_get_free
	};
	struct simple_allocator b = {
		simple_alloc, simple_free, simple_size
	};
	set_page_allocator(&a);
	set_simple_allocator(&b);
	int pt[4096/4];
	simple_allocator_bootstrap(pt, 4096);
	page_allocator_init();
	simple_allocator_init();
	page_allocator_move();
}

void test_syscall(){
	asm("mov $1, %eax; int $0x80;");
}
void test_alloc(){
	int i;
	int address[100];
	int j = 20;
	while(j--){
		for(i = 0; i < 100; ++i){
			uint32_t x = (uint32_t)kmalloc(20, 0);
			address[i] = x;
			kprintf("at 0x%x is 0x%x", x, *(int*)x);
			*(int*)x = j;
			kprintf(",now is 0x%x\n", *(int*)x);
		}
		for(i = 0; i < 100; ++i){
			kfree((void*)address[i]);
			kprintf("freed 0x%x\n", address[i]);
		}
	}
}

lock_t testlock;
int num;
void test_lock(){
	while(1){
		spin_lock(&testlock);
		kprintf("%d: cpu%d, ---abcdefghijklmnopqrstuvwxyz---\n", num, cpuid());
		num++;
		spin_unlock(&testlock);
	}
}


extern void mpinit();
extern void lapicinit();
extern void picinit();
extern void ioapicinit();
extern void switch_regs(struct context *old, struct context *new);
void high_address_entry(){
	allocator_init();
	mpinit();
	lapicinit();     // interrupt controller
	picinit();       // another interrupt controller
	ioapicinit();    // another interrupt controller
	trap_init();
	do_initcalls();
	//test_syscall();
	//test_alloc();
	//spinlock_init(&testlock);
	smp_startup();
	//test_lock();

	// test switch_regs()
	// struct context c = {1000, 1001, 1002, 1003, 1004};
	// switch_regs(&(current_proc->context), &c);

	panic("Finished All the Code !\n");
}
