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

#define PageBuddy(page) test_bit(PG_buddy, &(page)->flags)

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

static inline struct page* 
__page_find_buddy(struct page* page, uint32_t page_idx, unsigned order){
	uint32_t buddy_idx = page_idx ^ (1 << order);
	return page + (buddy_idx - page_idx);
}

static inline int
page_is_buddy(struct page* page, struct page* buddy, int order){
	if(page_zone_id(page) != page_zone_id(buddy))
		return 0;
	if(PageBuddy(buddy) && page_order(buddy) == order){
		return 1;
	}
	return 0;
}


