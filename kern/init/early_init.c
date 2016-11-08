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
void test();
__noreturn
void master_early_init(void)
{
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

void lapicinit();
void picinit();
void ioapicinit();
extern uint32_t early_init_start;
extern uint32_t early_init_end;
extern uint32_t norm_init_start;
extern uint32_t norm_init_end;
extern uint32_t late_init_start;
extern uint32_t late_init_end;
void high_address_entry(){

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
	lapicinit();     // interrupt controller
	picinit();       // another interrupt controller
	ioapicinit();    // another interrupt controller
	trap_init();
	asm("mov $1, %eax; int $0x80;");
	kpdebug("early_init_start = 0x%x\n", &early_init_start);
	kpdebug("early_init_end = 0x%x\n", &early_init_end);
	kpdebug("norm_init_start = 0x%x\n", &norm_init_start);
	kpdebug("norm_init_end = 0x%x\n", &norm_init_end);
	kpdebug("late_init_start = 0x%x\n", &late_init_start);
	kpdebug("late_init_end = 0x%x\n", &late_init_end);
	do_initcalls();
	panic("succeed !");
}
