// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

// uint* refcnt;
struct spinlock pgreflock; // XXX:用于 pageref 数组的锁，防止竞态条件引起内存泄漏
#define PA2PGREF_ID(p) (((p)-KERNBASE)/PGSIZE)  // TODO：为什么KERNBASE
#define PGREF_MAX_ENTRIES PA2PGREF_ID(PHYSTOP)
int refcnt[PGREF_MAX_ENTRIES];

#define PA2PGREF(p) refcnt[PA2PGREF_ID((uint64)(p))]

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
  initlock(&pgreflock, "pgref"); // 初始化锁
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  // uint sum = 0;
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE){
    kfree(p);
    // sum++;
  }
  // 初始化引用数组
  // uint cnt_t[sum];
  // memset(cnt_t, 0, sizeof(cnt_t));
  // refcnt = cnt_t;
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&pgreflock);
  // XXX: 由于锁不能嵌套
  // uint res = kpagerefinc(pa, -1);
  // if(res <= 0) {
  if(--PA2PGREF(pa) <= 0) {
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  release(&pgreflock);
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
    PA2PGREF(r) = 1;
  }
  // kpagerefinc(r, 1);

  return (void*)r;
}

// 为对应页面增加/减少引用计数
uint 
kpagerefinc(void *pa, int a)
{
  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("krefinc pa invalid");
  // XXX: 自己写的这个索引有问题，应该是KERNBASE？
  // int ind = ((char*)pa - (char*)PGROUNDUP((uint64)end)) / PGSIZE;
  acquire(&pgreflock);
  // refcnt[ind] += a;
  PA2PGREF(pa)++;
  release(&pgreflock);
  // return refcnt[ind];
  return PA2PGREF(pa);
}


// 当引用已经小于等于 1 时，不创建和复制到新的物理页，而是直接返回该页本身
// https://blog.miigon.net/posts/s081-lab6-copy-on-write-fork/
// 锁方面不想调了
void *kcopy_n_deref(void *pa) {
  acquire(&pgreflock);

  if(PA2PGREF(pa) <= 1) { // 只有 1 个引用，无需复制
    release(&pgreflock);
    return pa;
  }

  // 分配新的内存页，并复制旧页中的数据到新页
  uint64 newpa = (uint64)kalloc();
  if(newpa == 0) {
    release(&pgreflock);
    return 0; // out of memory
  }
  memmove((void*)newpa, (void*)pa, PGSIZE);

  // 旧页的引用减 1
  PA2PGREF(pa)--;

  release(&pgreflock);
  return (void*)newpa;
}