// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define SUPERPAGE_SIZE  (10 * SUPERPGSIZE) 
#define superpage_end   PHYSTOP
#define superpage_start (superpage_end - SUPERPAGE_SIZE)

void freerange(void *pa_start, void *pa_end);
void superfree(void *pa);
extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// 1. 专门为 2MB 大页设计的链表节点
struct super_run {
  struct super_run *next;
};

// 2. 专属的大页仓库
struct {
  struct spinlock lock;
  struct super_run *freelist; // 换上大页专属的制服！
} super_kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&super_kmem.lock, "super_kmem");

  // 直接使用宏进行范围初始化
  freerange(end, (void*)superpage_start);

  char *p = (char*)superpage_start;
  for(; p + SUPERPGSIZE <= (char*)superpage_end; p += SUPERPGSIZE) {
    superfree(p); 
  }

  freerange((void*)superpage_end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

// 如果未对齐，或者超出了物理内存上下限，或者不小心掉进了大页特区，统统 panic！
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP || 
     ((uint64)pa >= superpage_start && (uint64)pa < superpage_end)) {
    panic("kfree");
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

void
superfree(void *pa)
{
  struct super_run *r;

  if(((uint64)pa % SUPERPGSIZE) != 0 || (char*)pa < (char*)superpage_start || (uint64)pa >= superpage_end)
    panic("superfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, SUPERPGSIZE);

  r = (struct super_run*)pa;

  acquire(&super_kmem.lock);
  r->next = super_kmem.freelist;
  super_kmem.freelist = r;
  release(&super_kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
void *
superalloc(void)
{
  struct super_run *r;

  acquire(&super_kmem.lock);
  r = super_kmem.freelist;
  if(r)
    super_kmem.freelist = r->next;
  release(&super_kmem.lock);

  if(r)
    memset((char*)r, 5, SUPERPGSIZE); // fill with junk
  return (void*)r;
}