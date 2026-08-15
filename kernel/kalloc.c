// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];;

void
kinit()
{
  for(int i = 0; i < NCPU; i++) {
    // 为每个 CPU 的锁起名为 "kmem"，测试脚本会专门检查这个名字
    initlock(&kmem[i].lock, "kmem"); 
  }
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  push_off(); //关中断
  int id = cpuid(); //获取当前所在的cpu id

  acquire(&kmem[id].lock); //获取当前 CPU 的锁
  r->next = kmem[id].freelist; //挂回到当前 CPU 的专属链表
  kmem[id].freelist = r; 
  release(&kmem[id].lock); //释放当前 CPU 的锁

  pop_off(); //开中断
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
// Allocate one 4096-byte page of physical memory.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int id = cpuid();

  // 1. 先尝试从自己的金库拿
  acquire(&kmem[id].lock);
  r = kmem[id].freelist;
  if(r)
    kmem[id].freelist = r->next;
  release(&kmem[id].lock);

  // 2. 如果自己没钱了，去邻居那里偷
  if(r == 0){
    for(int j = 1; j < NCPU; j++){
      int i = (id + j) % NCPU; 
      
      // 无锁前置探测：如果邻居是个穷光蛋，直接跳过，连门都别敲！
      if(kmem[i].freelist != 0){
        
        acquire(&kmem[i].lock);
        r = kmem[i].freelist;
        
        if(r){
          // 【核心绝杀】：O(1) 偷窃，不数数，直接全盘端走！
          // 把目标的 freelist 直接置空，拿完立刻解锁，耗时趋近于 0。
          kmem[i].freelist = 0; 
          release(&kmem[i].lock);

          // 我们把偷来的第一页 r 留给用户。
          // 剩下的全塞进自己的金库。
          if(r->next){
            acquire(&kmem[id].lock);
            kmem[id].freelist = r->next;
            release(&kmem[id].lock);
          }
          break; // 成功偷到，全身而退
        }
        
        // 如果点背被别人抢先了，解锁继续找下一家
        release(&kmem[i].lock);
      }
    }
  }
  
  pop_off();

  if(r)
    memset((char*)r, 5, PGSIZE); 
  return (void*)r;
}
