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

#define NODE_UNUSED 0
#define NODE_USED 1	
#define NODE_SPLIT 2
#define NODE_FULL 3

int area_n = 0;
void test(){
    PMB* pmb = (PMB*)0x9004;
    int num = *(int *)0x9000;
    int i, largest = 0;
    int tmp = 0;
    kprintf("physical memory areas:\n");
    kprintf("BaseAddr\tLength\t\tType(1-usable by OS, 2-reserved address)\n");
    for(i = 0; i < num; ++i){
        kprintf("0x%8x\t0x%8x\t0x%x\n", 
                pmb->BaseAddrLow,
                pmb->LengthLow,
                pmb->Type);
        if(pmb->Type == 1 && pmb->LengthLow > tmp){
		tmp = pmb->LengthLow;
		largest = i;
        }
        pmb++;
    }
}

struct buddy {
	int level;
	uint8_t tree[1000];
};

struct buddy my_buddy;

void buddy_init(int level) {
	int size = 1 << level;
	my_buddy->level = level;
	memset(my_buddy->tree , NODE_UNUSED , size*2-1);
	return self;
}
