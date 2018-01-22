/*
** Copyright (c) 2014-2018 uboss.org All rights reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Service Handle
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/
#ifndef UBOSS_HANDLE_H
#define UBOSS_HANDLE_H

#include <stdint.h>

// 保留高8bit作为远程消息的编号
#define HANDLE_MASK 0xFFFFFF
#define HANDLE_REMOTE_SHIFT 24

//
// 读写锁的结构
struct rwlock {
	int write; // 写
	int read; // 读
};

// 初始化读写锁
static inline void
rwlock_init(struct rwlock *lock) {
	lock->write = 0;
	lock->read = 0;
}

// 读锁
static inline void
rwlock_rlock(struct rwlock *lock) {
	for (;;) {
		while(lock->write) {
			__sync_synchronize();
		}
		__sync_add_and_fetch(&lock->read,1);
		if (lock->write) {
			__sync_sub_and_fetch(&lock->read,1);
		} else {
			break;
		}
	}
}

// 写锁
static inline void
rwlock_wlock(struct rwlock *lock) {
	while (__sync_lock_test_and_set(&lock->write,1)) {}
	while(lock->read) {
		__sync_synchronize();
	}
}

// 解写锁
static inline void
rwlock_wunlock(struct rwlock *lock) {
	__sync_lock_release(&lock->write);
}

// 解读锁
static inline void
rwlock_runlock(struct rwlock *lock) {
	__sync_sub_and_fetch(&lock->read,1);
}

// 在 uboss_context.h 中定义
struct uboss_context;

uint32_t uboss_handle_register(struct uboss_context *);
int uboss_handle_retire(uint32_t handle);
struct uboss_context * uboss_handle_grab(uint32_t handle);
void uboss_handle_retireall();

uint32_t uboss_handle_findname(const char * name);
const char * uboss_handle_namehandle(uint32_t handle, const char *name);

void uboss_handle_init(int harbor);

#endif /* UBOSS_HANDLE_H */
