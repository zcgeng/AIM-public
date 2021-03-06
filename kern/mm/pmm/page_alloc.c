/*************************************************************************
	> File Name: page_alloc.c
	> Author: Zhao Changgeng
	> Mail: zcg1996@gmail.com
	> Created Time: 2016年10月25日 星期二 22时31分48秒
 ************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/param.h>
//#include <aim/sync.h>
#include <aim/mmu.h>
#include <aim/pmm.h>
#include <aim/vmm.h>
#include <arch-mmu.h>
#include <aim/panic.h>
#include <libc/string.h>
#include <util.h>
#include <aim/console.h>
void *kmalloc(size_t size, gfp_t flags);

typedef struct physical_memory_block{
    uint32_t BaseAddrLow; /* Low 32 Bits of Base Address */
    uint32_t BaseAddrHigh; /* High 32 Bits of Base Address */
    uint32_t LengthLow; /* Low 32 Bits of Length in Bytes */
    uint32_t LengthHigh; /* High 32 Bits of Length in Bytes */
    uint32_t Type; /* Address type of  this range */
}PMB;

struct buddy {
	int level;
	int baseAddr;
	uint8_t tree[1 << 10]; // fixed buddy size: 4M
};

struct Area{
	uint32_t base_addr;
	uint32_t page_num;
	uint32_t buddy_num;
	struct buddy** buddylist;
};

struct Area area;

#define NODE_UNUSED 0
#define NODE_USED 1
#define NODE_SPLIT 2
#define NODE_FULL 3

static inline int
is_pow_of_2(uint32_t x) {
	return !(x & (x-1));
}

static inline uint32_t
next_pow_of_2(uint32_t x) {
	if ( is_pow_of_2(x) )
		return x;
	x |= x>>1;
	x |= x>>2;
	x |= x>>4;
	x |= x>>8;
	x |= x>>16;
	return x+1;
}

static inline int get_level(uint32_t page_num){
	int i = 0;
	for(i = 0; page_num >> i != 0; i++);
	if(is_pow_of_2(page_num))
		return i - 2;
	else
		return i;
}

void buddy_init(struct buddy* self, int level);
void buddy_free(struct buddy* self, int offset);
int buddy_alloc(struct buddy* self, int s);
int buddy_size(struct buddy * self, int offset);

int page_alloc(struct pages *pages){
	//kprintf("page_alloc: pages->size=%d\n", pages->size);
	int i, offset = -1;
	int s = (pages->size + PGSIZE - 1) >> 12;
	for(i = 0; i < area.buddy_num; ++i){
		offset = buddy_alloc(area.buddylist[i], s);
		if(offset >= 0) break;
	}
	if(offset == -1){
		if(area.buddy_num >= area.page_num / 1024) return EOF;
		buddy_init(area.buddylist[area.buddy_num++], 10);
		offset = buddy_alloc(area.buddylist[i], s);
	}
	if(offset == -2 || offset == -1) return EOF;
  //kprintf("base_addr=0x%x, i=%d, offset=0x%x\n",area.base_addr,  i, offset);
	pages->paddr = area.base_addr + (i << 22) + (offset << 12); // each buddy has 4MB size and each offset is 4KB
	area.page_num -= s;
	return 0;
}

void page_free(struct pages *pages) {
	int buddyN = (pages->paddr - area.base_addr) >> 22;
	int offset = ((pages->paddr - area.base_addr) >> 12) % 1024;
	pages->size = buddy_size(area.buddylist[buddyN], offset) << 12;
	if(pages->size > 0)
		area.page_num += pages->size;
}

addr_t page_get_free(void){
	return area.page_num << 12;
}

addr_t mem_phybase = 0;
addr_t mem_size = 0;
void get_mem_init(){
  // get memory infomation saved by boot loader
	PMB* pmb = (PMB*)0x9004;
	int num = *(int *)0x9000;
	int i, largest = 0;
	int tmp = 0;
	kprintf("physical memory areas:\n");
	kprintf("BaseAddr\tLength\t\tType(1-usable by OS, 2-reserved address)\n");
	for(i = 0; i < num; ++i){
        kprintf("0x%8x\t0x%8x\t0x%x\n",
                pmb[i].BaseAddrLow,
                pmb[i].LengthLow,
                pmb[i].Type);
		if(pmb[i].Type == 1 && pmb[i].LengthLow > tmp){
			tmp = pmb[i].LengthLow;
			largest = i;
		}
	}
  mem_phybase = pmb[largest].BaseAddrLow;
  mem_size = pmb[largest].LengthLow;
}

addr_t get_mem_physbase(){
  if(mem_phybase == 0) get_mem_init();
  return mem_phybase;
}

addr_t get_mem_size(){
  if(mem_size == 0) get_mem_init();
  return mem_size;
}

extern uint32_t _end;
int page_allocator_init(void){
  get_mem_init();
	area.base_addr = get_mem_physbase();
	area.page_num = get_mem_size() / 4096;
	uint32_t end = premap_addr((uint32_t)&_end);
	if(area.base_addr < end){
		area.base_addr = PGROUNDUP(end);
		area.page_num -= (area.base_addr - end) / 4096;
	}
	area.buddylist = (struct buddy**)kmalloc(sizeof(void *) * (area.page_num / 1024), 0);
	area.buddy_num = 0;
	return 0;
}

void buddy_init(struct buddy* self, int level) {
	int size = 1 << level;
	self->level = level;
	if(self == NULL)
		self = (struct buddy*)kmalloc(sizeof(struct buddy), 0);
	//kprintf("level=%d, size=0x%x\n", self->level, size);
	memset(self->tree , NODE_UNUSED , size*2-1);
	return;
}



/* get the page number of a tree node */
static inline int
_index_offset(int index, int level, int max_level) {
	return ((index + 1) - (1 << level)) << (max_level - level);
}

static inline int
get_buddy_index(int index){
	return index - 1 + (index & 1) * 2;
}

static void
_mark_parent(struct buddy* self, int index) {
	for (;;) {
		int buddy = get_buddy_index(index);
		if (buddy > 0 && (self->tree[buddy] == NODE_USED || self->tree[buddy] == NODE_FULL)) {
			index = (index + 1) / 2 - 1;
			self->tree[index] = NODE_FULL;
		}
		else {
			return;
		}
	}
}

int buddy_alloc(struct buddy* self, int s) {
	//kprintf("buddy_alloc: s=%d\n", s);
	int size;
	if (s==0) {
		size = 1;
	} else {
		size = (int)next_pow_of_2(s);
	}
	int length = 1 << self->level;
	//kprintf("length=%d\n", length);
	if (size > length){
		kprintf("too large size:0x%x (length=0x%x)\n", size, length);
		return -2;
	}

	int index = 0;
	int level = 0;

	// find size from the root
	while (index >= 0) {
		if (size == length) {
			if (self->tree[index] == NODE_UNUSED) {
				self->tree[index] = NODE_USED;
				_mark_parent(self, index);
				return _index_offset(index, level, self->level);
			}
		} else {
			// size < length
			switch (self->tree[index]) {
			case NODE_USED:
			case NODE_FULL:
				break;
			case NODE_UNUSED:
				// split first
				self->tree[index] = NODE_SPLIT;
				self->tree[index*2+1] = NODE_UNUSED;
				self->tree[index*2+2] = NODE_UNUSED;
			default:
				index = index * 2 + 1;
				length /= 2;
				level++;
				continue;
			}
		}
		if (index & 1) {
			// search its buddy
			++index;
			continue;
		}

		/* go back to the parent node */
		for (;;) {
			level--;
			length *= 2;
			index = (index+1)/2 -1;
			if (index < 0)
				return -1;
			if (index & 1) {
				++index;
				break;
			}
		}
	}

	return -1;
}

static void
_combine(struct buddy* self, int index) {
	for (;;) {
		int buddy = index - 1 + (index & 1) * 2;
		if (buddy < 0 || self->tree[buddy] != NODE_UNUSED) {
			self->tree[index] = NODE_UNUSED;
			while (((index = (index + 1) / 2 - 1) >= 0) &&  self->tree[index] == NODE_FULL){
				self->tree[index] = NODE_SPLIT;
			}
			return;
		}
		index = (index + 1) / 2 - 1;
	}
}

void buddy_free(struct buddy* self, int offset) {
	if( offset >= (1<< self->level))
		panic("offset 0x%x is larger than %d levels of buddy!", offset, self->level);
	int left = 0;
	int length = 1 << self->level;
	int index = 0;

	for (;;) {
		switch (self->tree[index]) {
		case NODE_USED:
			if(offset != left)
				panic("offset(0x%x) != left(0x%x)!\n", offset, left);
			_combine(self, index);
			return;
		case NODE_UNUSED:
			panic("Trying to free an unused block!\n");
			return;
		default:
			length /= 2;
			if (offset < left + length) {
				index = index * 2 + 1;
			} else {
				left += length;
				index = index * 2 + 2;
			}
			break;
		}
	}
}

int buddy_size(struct buddy * self, int offset) {
	if( offset >= (1<< self->level)) return EOF;
	int left = 0;
	int length = 1 << self->level;
	int index = 0;

	for (;;) {
		switch (self->tree[index]) {
		case NODE_USED:
			return length;
		case NODE_UNUSED:
			return EOF;
		default:
			length /= 2;
			if (offset < left + length) {
				index = index * 2 + 1;
			} else {
				left += length;
				index = index * 2 + 2;
			}
			break;
		}
	}
}

int page_allocator_move(){
	return 0;
}
