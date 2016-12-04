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
#include <aim/early_kmmap.h>
#include <aim/mmu.h>
#include <asm.h>
#include <libc/string.h>
#include <arch-mmu.h>
#include <aim/memlayout.h>
#include <aim/pmm.h>
#include <aim/panic.h>

__attribute__((__aligned__(PGSIZE)))
pgindex_t entrypgdir[NPDENTRIES];

extern uint32_t _end;
/* Initialize a page index table and fill in the structure @pgindex */
void early_mm_init(void){
	page_index_early_map((pgindex_t*)V2P(entrypgdir), 0, (void*)0, (uint32_t)&_end - KERN_BASE + 0x10000000);
	page_index_early_map((pgindex_t*)V2P(entrypgdir), 0, P2V(0), (uint32_t)&_end - KERN_BASE + KERN_START + 0x10000000);
	page_index_early_map((pgindex_t*)V2P(entrypgdir), DEVSPACE, (void*)DEVSPACE, 0x01000000);
}

/* Map virtual address starting at @vaddr to physical pages at @paddr, with
 * VMA flags @flags (VMA_READ, etc.) Additional flags apply as in arch-mmu.h
 */
int page_index_early_map(pgindex_t *boot_page_index, addr_t paddr, void *vaddr, size_t size){
	uint32_t a, last;
	a = PGROUNDDOWN((uint32_t)vaddr);
	last = PGROUNDDOWN(((uint32_t)vaddr) + size - 1);
	while(a < last){
		boot_page_index[a >> PDXSHIFT] = paddr | PTE_P | PTE_W | PTE_PS;
		a += (1 << 22);
		paddr += (1 << 22);
	}
	return (last - a) >> 22;
}

bool early_mapping_valid(struct early_mapping *entry)
{
	return true;
}

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
pte_t *
walkpgdir(pgindex_t *pgdir, const void *va, int alloc)
{
  pgindex_t *pde;
  pgindex_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)postmap_addr(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)(int)pgalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = premap_addr(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

void page_index_clear(pgindex_t *index){
}

int map_pages(pgindex_t *pgindex, void *vaddr, addr_t paddr, size_t size, uint32_t flags){
	char *a, *last;
  pte_t *pte;

  a = (char*)PGROUNDDOWN((uint)vaddr);
  last = (char*)PGROUNDDOWN(((uint)vaddr) + size - 1);
  for(;;){
    if((pte = walkpgdir(pgindex, a, 1)) == 0)
      return -1;
    if(*pte & PTE_P)
      panic("remap");
    *pte = paddr | flags | PTE_P;
    if(a == last)
      break;
    a += PGSIZE;
    paddr += PGSIZE;
  }
  return 0;
}

ssize_t unmap_pages(pgindex_t *pgindex, void *vaddr, size_t size, addr_t *paddr){
	return -1;
}

pgindex_t *init_pgindex(void){
	return NULL;
}

void destroy_pgindex(pgindex_t *pgindex){
}

int set_pages_perm(pgindex_t *pgindex, void *vaddr, size_t size, uint32_t flags){
	return -1;
}

pgindex_t *get_pgindex(void){
	uint paddr;
	asm(
		"mov %%cr3, %0;"
		:"=r"(paddr)
	);
	return (pgindex_t*)V2P(paddr);
}

int switch_pgindex(pgindex_t *pgindex){
	/* Set page directory */
	asm(
		"mov    %0, %%cr3;"
		::"r"(V2P(pgindex))
	);
	return 0;
}

void mmu_init(pgindex_t *boot_page_index)
{
	/* Turn on page size extension for 4Mbyte pages */
	asm(
		"mov    %%cr4, %%eax;"
		"or     %0, %%eax;"
		"mov    %%eax, %%cr4;"
		::"i"(CR4_PSE)
	);
	switch_pgindex(boot_page_index);
	/* Turn on paging */
	asm(
		"mov	%%cr0, %%eax;"
		"or	%0, %%eax;"
		"mov	%%eax, %%cr0;"
		::"i"(CR0_PG | CR0_WP)
	);
}

__noreturn
void abs_jump(void *addr){
	jmp(addr);
}
