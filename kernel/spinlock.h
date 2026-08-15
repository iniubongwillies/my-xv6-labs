// Mutual exclusion lock.
struct spinlock {
  uint locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
#ifdef LAB_LOCK
  int nts;
  int n;
#endif
};

#ifdef LAB_LOCK
// Reader-writer lock.
struct rwspinlock {
  struct spinlock l; // 用于保护内部状态的底层互斥锁
  int count;         // 当前正在读的数量
  int flag;          // 当前是否有写者正在写 (0 或 1)
  int wait;          // 正在排队的写者数量
};
#endif
