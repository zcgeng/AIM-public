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
#include <aim/mmu.h>
#include <arch-mmu.h>

void mmu_init(pgindex_t *boot_page_index)
{
}

/* Initialize a page index table and fill in the structure @pgindex */
//pgindex_t *init_pgindex(void){
	
//}

/* Map virtual address starting at @vaddr to physical pages at @paddr, with
 * VMA flags @flags (VMA_READ, etc.) Additional flags apply as in kmmap.h.
 */
int map_pages(pgindex_t *pgindex, void *vaddr, addr_t paddr, size_t size, uint32_t flags){
	char *a, *last;
	a = (char*)PGROUNDDOWN((uint)vaddr);
	last = (char*)PGROUNDDOWN(((uint)vaddr) + size - 1);
	while(a < last){
		*pgindex = paddr | flags | PTE_P;
		a += PGSIZE;
		paddr += PGSIZE;
	}
	return 0;
}




