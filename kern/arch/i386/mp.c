// Multiprocessor support
// Search memory for MP description structures.
// http://developer.intel.com/design/pentium/datashts/24201606.pdf

#include "sys/types.h"
#include "aim/memlayout.h"
#include "mp.h"
#include "aim/proc.h"
#include "asm.h"
#include "arch-mmu.h"
#include "libc/string.h"
#include "aim/console.h"
#include "aim/vmm.h"
#include "aim/panic.h"

struct percpu cpus[NCPU];
int ismp;
int ncpu;
uchar ioapicid;
extern volatile uint *lapic;

static uchar sum(uchar *addr, int len)
{
  int i, sum;

  sum = 0;
  for(i=0; i<len; i++)
    sum += addr[i];
  return sum;
}

// Look for an MP structure in the len bytes at addr.
static struct mp*
mpsearch1(uint a, int len)
{
  uchar *e, *p, *addr;

  addr = P2V(a);
  e = addr+len;
  for(p = addr; p < e; p += sizeof(struct mp))
    if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
      return (struct mp*)p;
  return 0;
}

// Search for the MP Floating Pointer Structure, which according to the
// spec is in one of the following three locations:
// 1) in the first KB of the EBDA;
// 2) in the last KB of system base memory;
// 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
static struct mp*
mpsearch(void)
{
  uchar *bda;
  uint p;
  struct mp *mp;

  bda = (uchar *) P2V(0x400);
  if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
    if((mp = mpsearch1(p, 1024)))
      return mp;
  } else {
    p = ((bda[0x14]<<8)|bda[0x13])*1024;
    if((mp = mpsearch1(p-1024, 1024)))
      return mp;
  }
  return mpsearch1(0xF0000, 0x10000);
}

// Search for an MP configuration table.  For now,
// don't accept the default configurations (physaddr == 0).
// Check for correct signature, calculate the checksum and,
// if correct, check the version.
// To do: check extended table checksum.
static struct mpconf*
mpconfig(struct mp **pmp)
{
  struct mpconf *conf;
  struct mp *mp;

  if((mp = mpsearch()) == 0 || mp->physaddr == 0)
    return 0;
  conf = (struct mpconf*) P2V((uint) mp->physaddr);
  if(memcmp(conf, "PCMP", 4) != 0)
    return 0;
  if(conf->version != 1 && conf->version != 4)
    return 0;
  if(sum((uchar*)conf, conf->length) != 0)
    return 0;
  *pmp = mp;
  return conf;
}

void
mpinit(void)
{
  uchar *p, *e;
  struct mp *mp;
  struct mpconf *conf;
  struct mpproc *proc;
  struct mpioapic *ioapic;

  if((conf = mpconfig(&mp)) == 0)
    return;
  ismp = 1;
  lapic = (uint*)conf->lapicaddr;
  for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
    switch(*p){
    case MPPROC:
      proc = (struct mpproc*)p;
      if(ncpu < NCPU) {
        cpus[ncpu].apicid = proc->apicid;  // apicid may differ from ncpu
        ncpu++;
      }
      p += sizeof(struct mpproc);
      continue;
    case MPIOAPIC:
      ioapic = (struct mpioapic*)p;
      ioapicid = ioapic->apicno;
      p += sizeof(struct mpioapic);
      continue;
    case MPBUS:
    case MPIOINTR:
    case MPLINTR:
      p += 8;
      continue;
    default:
      ismp = 0;
      break;
    }
  }
  if(!ismp){
    // Didn't like what we found; fall back to no MP.
    ncpu = 1;
    lapic = 0;
    ioapicid = 0;
    return;
  }

  if(mp->imcrp){
    // Bochs doesn't support IMCR, so this doesn't run on Bochs.
    // But it would on real hardware.
    outb(0x22, 0x70);   // Select IMCR
    outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
  }
}


int cpunum();
// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void seginit(void)
{
  struct percpu *c;

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
  c->gdt[SEG_KCPU] = SEG(STA_W, &c->percpu, 8, 0);

  lgdt(c->gdt, sizeof(c->gdt));
  loadgs(SEG_KCPU << 3);

  // Initialize cpu-local storage.
  set_gs_cpu(c);
  set_gs_proc(0);
}

void trap_init();
extern void test_lock();
// Common CPU setup code.
static void mpmain(void)
{
  kprintf("cpu%d: starting...\n", cpunum());
  trap_init();       // load idt register
  xchg(&get_gs_cpu()->started, 1); // tell startothers() we're up
  kprintf("cpu%d: started!\n", cpunum());
  sti();
  //test_lock();
	while(1){
    asm("hlt;");
  }
  //scheduler();     // start running processes
}

void seginit();
void lapicinit();
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

int nr_cpus(void){
  return ncpu;
}

int cpuid(){
  struct percpu* c = get_gs_cpu();
  if(c == NULL) return 0;
  return c->apicid;
}

// Start the non-boot (AP) processors.
void startothers(void)
{
  extern uchar _binary_entryother_start[], _binary_entryother_end[];
	uint _binary_entryother_size = (uint)_binary_entryother_end - (uint)_binary_entryother_start;
  uchar *code;
  struct percpu *c;
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
    while(c->started == 0);
  }
}

void set_gs_cpu(struct percpu* c){
  asm(
    "mov %0, %%eax;"
    "mov %%eax, %%gs:0"
    ::"m"(c)
  );
}

struct percpu* get_gs_cpu(){
  struct percpu* c;
  asm(
    "mov %%gs:0, %%eax;"
    "mov %%eax, %0"
    :"=m"(c)
  );
  return c;
}

void set_gs_proc(struct proc* proc){
  asm(
    "mov %0, %%eax;"
    "mov %%eax, %%gs:4"
    ::"m"(proc)
  );
}

struct proc* get_gs_proc(){
  struct proc* proc;
  asm(
    "mov %%gs:4, %%eax;"
    "mov %%eax, %0"
    :"=m"(proc)
  );
  return proc;
}

void smp_startup(void){
  startothers();
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
