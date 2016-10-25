/*************************************************************************
	> File Name: simple_alloc.c
	> Author: Zhao Changgeng
	> Mail: zcg1996@gmail.com
	> Created Time: 2016年10月25日 星期二 14时12分38秒
 ************************************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <sys/param.h>
//#include <aim/sync.h>
#include <aim/vmm.h>
#include <aim/mmu.h>	/* PAGE_SIZE */
#include <aim/panic.h>
#include <libc/string.h>
/* 
 * A block header. This is in front of every malloc-block, whether free or not.
 */
struct block_header {
	unsigned long bh_flags;
	union {
		unsigned long ubh_length;
		struct block_header *fbh_next;
	} vp;
};


#define bh_length vp.ubh_length
#define bh_next   vp.fbh_next
#define BH(p) ((struct block_header *)(p))


/* 
 * The page descriptor is at the front of every page that malloc has in use. 
 */
struct page_descriptor {
	struct page_descriptor *next;
	struct block_header *firstfree;
	int order;
	int nfree;
};


#define PAGE_DESC(p) ((struct page_descriptor *)(((unsigned long)(p)) & PAGE_MASK))


/*
 * A size descriptor describes a specific class of malloc sizes.
 * Each class of sizes has its own freelist.
 */
struct size_descriptor {
	struct page_descriptor *firstfree;
	int size;
	int nblocks;

	int nmallocs;
	int nfrees;
	int nbytesmalloced;
	int npages;
};

struct size_descriptor sizes[] = { 
	{ NULL,  32,127, 0,0,0,0 },
	{ NULL,  64, 63, 0,0,0,0 },
	{ NULL, 128, 31, 0,0,0,0 },
	{ NULL, 252, 16, 0,0,0,0 },
	{ NULL, 508,  8, 0,0,0,0 },
	{ NULL,1020,  4, 0,0,0,0 },
	{ NULL,2040,  2, 0,0,0,0 },
	{ NULL,4080,  1, 0,0,0,0 },
	{ NULL,   0,  0, 0,0,0,0 }
};
#define NBLOCKS(order)          (sizes[order].nblocks)
#define BLOCKSIZE(order)        (sizes[order].size)

int simple_allocator_bootstrap(void *pt, size_t size){
	
}





