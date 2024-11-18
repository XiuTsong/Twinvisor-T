#ifndef __S_VISOR_LOCK_H__
#define __S_VISOR_LOCK_H__

struct lock {
	volatile unsigned long long slock;
};

int lock_init(struct lock *lock);
void lock(struct lock *lock);
/* returns 0 on success, -1 otherwise */
int try_lock(struct lock *lock);
void unlock(struct lock *lock);
int is_locked(struct lock *lock);

#endif