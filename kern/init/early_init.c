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
#include <aim/memlayout.h>

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

extern void mpinit();
extern void lapicinit();
extern void picinit();
extern void ioapicinit();
static void startothers();

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
	startothers();
	panic("succeed !");
}

int cpunum();
// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void seginit(void)
{
  struct cpu *c;

  // Map "logical" addresses to virtual addresses using identity map.
  // Cannot share a CODE descriptor for both kernel and user
  // because it would have to have DPL_USR, but the CPU forbids
  // an interrupt from CPL=0 to DPL=3.
  c = &cpus[cpunum()];
  c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);

  // Map cpu and proc -- these are private per cpu.
  c->gdt[SEG_KCPU] = SEG(STA_W, &c->cpu, 8, 0);

  lgdt(c->gdt, sizeof(c->gdt));
  loadgs(SEG_KCPU << 3);

  // Initialize cpu-local storage.
  //cpu = c;
  //proc = 0;
}

// Common CPU setup code.
static void mpmain(void)
{
  kprintf("cpu%d: starting\n", cpunum());
  trap_init();       // load idt register
  //xchg(&cpu->started, 1); // tell startothers() we're up
  //scheduler();     // start running processes
}

// Other CPUs jump here from entryother.S.
static void mpenter(void)
{
  //switchkvm();
  seginit();
  lapicinit();
  mpmain();
}

pde_t entrypgdir[];  // For entry.S
void lapicstartap(uchar apicid, uint addr);
// Start the non-boot (AP) processors.
static void startothers(void)
{
  extern uchar _binary_entryother_start[], _binary_entryother_end[];
	uint _binary_entryother_size = (uint)_binary_entryother_end - (uint)_binary_entryother_start;
  uchar *code;
  struct cpu *c;
  char *stack = NULL;

  // Write entry code to unused memory at 0x7000.
  // The linker has placed the image of entryother.S in
  // _binary_entryother_start.
  code = P2V(0x7000);
  memcpy(code, _binary_entryother_start, (uint)_binary_entryother_size);
  for(c = cpus; c < cpus+ncpu; c++){
    if(c == cpus+cpunum())  // We've started already.
      continue;

    // Tell entryother.S what stack to use, where to enter, and what
    // pgdir to use. We cannot use kpgdir yet, because the AP processor
    // is running in low  memory, so we use entrypgdir for the APs too.
    stack = kmalloc(2048, 0);
    *(void**)(code-4) = stack + 2048;
    *(void**)(code-8) = mpenter;
    *(int**)(code-12) = (void *) V2P(entrypgdir);

    lapicstartap(c->apicid, V2P(code));

    // wait for cpu to finish mpmain()
    while(c->started == 0)
      ;
  }
}

// The boot page table used in entry.S and entryother.S.
// Page directories (and page tables) must start on page boundaries,
// hence the __aligned__ attribute.
// PTE_PS in a page directory entry enables 4Mbyte pages.

__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // Map VA's [0, 4MB) to PA's [0, 4MB)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  [KERN_BASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};
