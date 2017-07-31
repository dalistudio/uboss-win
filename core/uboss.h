/*
** Copyright (c) 2014-2017 uboss.org All rights reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Main Function
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in this file end.
*/

#ifndef UBOSS_H
#define UBOSS_H

#include <stddef.h>
#include <stdint.h>

#define UBOSS_VERSION_MAJOR	"3"
#define UBOSS_VERSION_MINOR	"0"
#define UBOSS_VERSION_RELEASE	"0"
#define UBOSS_VERSION_NUM	300

#define UBOSS_VERSION	"uBoss " UBOSS_VERSION_MAJOR "." UBOSS_VERSION_MINOR
#define UBOSS_RELEASE	UBOSS_VERSION "." UBOSS_VERSION_RELEASE
#define UBOSS_COPYRIGHT	UBOSS_RELEASE "  Copyright (c) 2014-2017 uboss.org  All rights reserved."
#define UBOSS_AUTHORS	"Dali Wang <dali@uboss.org>"

// 预留内存分配函数的钩子
// 为以后使用 jemalloc 做准备
#define uboss_malloc malloc
#define uboss_calloc calloc
#define uboss_realloc realloc
#define uboss_free free

// for uboss_lalloc use
#define raw_realloc realloc
#define raw_free free
void *uboss_lalloc(void *ud, void *ptr, size_t osize, size_t nsize);

/////////////////////////////////////////////////////////
// 原子操作

// 比较与交换
#define ATOM_CAS(ptr, oval, nval) __sync_bool_compare_and_swap(ptr, oval, nval)
#define ATOM_CAS_POINTER(ptr, oval, nval) __sync_bool_compare_and_swap(ptr, oval, nval)

// 先加1再取回
#define ATOM_INC(ptr) __sync_add_and_fetch(ptr, 1)

// 先取回再加1
#define ATOM_FINC(ptr) __sync_fetch_and_add(ptr, 1)

// 先减1再取回
#define ATOM_DEC(ptr) __sync_sub_and_fetch(ptr, 1)

// 先取回再减1
#define ATOM_FDEC(ptr) __sync_fetch_and_sub(ptr, 1)

// 先加n再取回
#define ATOM_ADD(ptr,n) __sync_add_and_fetch(ptr, n)

// 先减n再取回
#define ATOM_SUB(ptr,n) __sync_sub_and_fetch(ptr, n)

// 先加n再取回
#define ATOM_AND(ptr,n) __sync_and_and_fetch(ptr, n)
/////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////
// 锁

#define SPIN_INIT(q) spinlock_init(&(q)->lock);
#define SPIN_LOCK(q) spinlock_lock(&(q)->lock);
#define SPIN_UNLOCK(q) spinlock_unlock(&(q)->lock);
#define SPIN_DESTROY(q) spinlock_destroy(&(q)->lock);

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

/////////////////////////////////////////

// 自旋锁的结构
struct spinlock {
	int lock; // 锁
};

// 初始化 自旋锁
static inline void
spinlock_init(struct spinlock *lock) {
	lock->lock = 0; // 设置锁为0
}

// 锁住
static inline void
spinlock_lock(struct spinlock *lock) {
	while (__sync_lock_test_and_set(&lock->lock,1)) {}
}

// 尝试锁住
static inline int
spinlock_trylock(struct spinlock *lock) {
	return __sync_lock_test_and_set(&lock->lock,1) == 0;
}

// 解锁
static inline void
spinlock_unlock(struct spinlock *lock) {
	__sync_lock_release(&lock->lock);
}

// 销毁锁
static inline void
spinlock_destroy(struct spinlock *lock) {
	(void) lock;
}
/////////////////////////////////////////////////////////

#define PTYPE_TEXT 0 // 文本类型
#define PTYPE_RESPONSE 1 // 响应类型
#define PTYPE_MULTICAST 2 // 组播类型
#define PTYPE_CLIENT 3 // 客户端类型
#define PTYPE_SYSTEM 4 // 系统类型
#define PTYPE_HARBOR 5 // 集群类型
#define PTYPE_SOCKET 6 // 网络类型
#define PTYPE_ERROR 7 // 错误类型
#define PTYPE_RESERVED_QUEUE 8 // 队列保留类型
#define PTYPE_RESERVED_DEBUG 9 // 调试保留类型
#define PTYPE_RESERVED_LUA 10 // Lua保留类型

#define PTYPE_TAG_DONTCOPY 0x10000 // 不复制消息
#define PTYPE_TAG_ALLOCSESSION 0x20000 // 允许会话

struct uboss_context;

// 服务的回调函数定义
typedef int (*uboss_cb)(struct uboss_context * context, void *ud, int type, int session, uint32_t source , const void * msg, size_t sz);

// 调用服务的回调函数
void uboss_callback(struct uboss_context * context, void *ud, uboss_cb cb);

// 复制字符串
char * uboss_strdup(const char *str);


#endif /* UBOSS_H */
/*
The MIT License (MIT)

Copyright (c) 2014-2017 uboss.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 */
