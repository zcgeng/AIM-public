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
#include <arch-mmu.h>
#include <aim/panic.h>
#include <libc/string.h>
#include <util.h>
#include <aim/console.h>

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

typedef struct area{
	uint32_t base_addr;
	uint32_t page_num;
	struct buddy buddylist[10]; // TODO: change this into dynimic pointer
};



struct buddy my_buddy;

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

void buddy_init(int level, uint32_t baseAddr);
void test(){
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
    buddy_init(10, PGROUNDUP(pmb[largest].BaseAddrLow));
}



void buddy_init(int level, uint32_t baseAddr) {
	int size = 1 << level;
	my_buddy.level = level;
	my_buddy.baseAddr = baseAddr;
	kprintf("level=%d, baseaddr=0x%x, size=0x%x\n", level, baseAddr, size);
	memset(my_buddy.tree , NODE_UNUSED , size*2-1);
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
_mark_parent(int index) {
	for (;;) {
		int buddy = get_buddy_index(index);
		if (buddy > 0 && (my_buddy.tree[buddy] == NODE_USED || my_buddy.tree[buddy] == NODE_FULL)) {
			index = (index + 1) / 2 - 1;
			my_buddy.tree[index] = NODE_FULL;
		}
		else {
			return;
		}
	}
}

int 
buddy_alloc(int s) {
	int size;
	if (s==0) {
		size = 1;
	} else {
		size = (int)next_pow_of_2(s);
	}
	int length = 1 << my_buddy.level;

	if (size > length)
		return -1;

	int index = 0;
	int level = 0;

	// find size from the root
	while (index >= 0) {
		if (size == length) {
			if (my_buddy.tree[index] == NODE_UNUSED) {
				my_buddy.tree[index] = NODE_USED;
				_mark_parent(index);
				return _index_offset(index, level, my_buddy.level);
			}
		} else {
			// size < length
			switch (my_buddy.tree[index]) {
			case NODE_USED:
			case NODE_FULL:
				break;
			case NODE_UNUSED:
				// split first
				my_buddy.tree[index] = NODE_SPLIT;
				my_buddy.tree[index*2+1] = NODE_UNUSED;
				my_buddy.tree[index*2+2] = NODE_UNUSED;
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
_combine(int index) {
	for (;;) {
		int buddy = index - 1 + (index & 1) * 2;
		if (buddy < 0 || my_buddy.tree[buddy] != NODE_UNUSED) {
			my_buddy.tree[index] = NODE_UNUSED;
			while (((index = (index + 1) / 2 - 1) >= 0) &&  my_buddy.tree[index] == NODE_FULL){
				my_buddy.tree[index] = NODE_SPLIT;
			}
			return;
		}
		index = (index + 1) / 2 - 1;
	}
}

void buddy_free(int offset) {
	if( offset < (1<< my_buddy.level))
		panic("offset %d is too large!", offset);
	int left = 0;
	int length = 1 << my_buddy.level;
	int index = 0;

	for (;;) {
		switch (my_buddy.tree[index]) {
		case NODE_USED:
			if(offset == left)
				panic("offset == left == 0x%x!\n", offset);
			_combine(index);
			return;
		case NODE_UNUSED:
			panic("Trying to free an unused page!\n");
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