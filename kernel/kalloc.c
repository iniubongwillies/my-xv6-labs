// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

struct {
  struct spinlock lock;
  int count[MAXPAGES];
} ref;

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref.lock, "ref");
  freerange(end, (void*)PHYSTOP);
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

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // === 新增：处理引用计数 ===
  acquire(&ref.lock);
  // 如果引用计数大于 0，就减 1
  if(ref.count[PA2INDEX(pa)] > 0) {
    ref.count[PA2INDEX(pa)]--;
  }
  // 记录减完之后的计数值
  int c = ref.count[PA2INDEX(pa)];
  release(&ref.lock);

  // 如果减 1 之后，引用计数仍然大于 0，说明还有其他进程在用
  // 直接 return，绝对不能执行后面的 memset 和回收逻辑！
  if(c > 0)
    return;
  // ==========================

  // 只有当 c == 0 时，才会继续执行原本的回收逻辑
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
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

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&ref.lock);
    ref.count[PA2INDEX(r)] = 1;
    release(&ref.lock);}
    
  return (void*)r;
}

// 增加引用计数
void
krefinc(void *pa)
{
  acquire(&ref.lock);
  ref.count[PA2INDEX(pa)]++;
  release(&ref.lock);
}

// 获取引用计数
int
krefget(void *pa)
{
  int c;
  acquire(&ref.lock);
  c = ref.count[PA2INDEX(pa)];
  release(&ref.lock);
  return c;
}
