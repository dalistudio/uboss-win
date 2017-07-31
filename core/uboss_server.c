/*
** Copyright (c) 2014-2017 uboss.org All rights reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Server
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#include "uboss_server.h"

#include "uboss.h"
#include "uboss_context.h"
#include "uboss_handle.h"
#include "uboss_module.h"
#include "uboss_mq.h"
#include "uboss_monitor.h"
#include "uboss_timer.h"
#include "uboss_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

void *
uboss_lalloc(void *ud, void *ptr, size_t osize, size_t nsize) {
	if (nsize == 0) {
		raw_free(ptr);
		return NULL;
	} else {
		return raw_realloc(ptr, nsize);
	}
}

#if defined(__WIN32__)
// 添加strsep函数
char 
*strsep(char **stringp, const char *delim)
{
    char *s;
    const char *spanp;
    int c, sc;
    char *tok;
    if ((s = *stringp)== NULL)
        return (NULL);
    for (tok = s;;) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc =*spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}
#endif

// 复制字符串
char *
uboss_strdup(const char *str) {
	size_t sz = strlen(str);
	char * ret = uboss_malloc(sz+1);
	memcpy(ret, str, sz+1);
	return ret;
}

// 全局线程管道初始化
void
uboss_globalinit(void) {
	G_NODE.total = 0;
	G_NODE.monitor_exit = 0;
	G_NODE.init = 1;
	if (pthread_key_create(&G_NODE.handle_key, NULL)) {
		fprintf(stderr, "pthread_key_create failed");
		exit(1);
	}
	// set mainthread's key
	uboss_initthread(THREAD_MAIN); // 初始化主线程
}

// 退出全局线程管道
void
uboss_globalexit(void) {
	pthread_key_delete(G_NODE.handle_key);
}

// 初始化线程管道
void
uboss_initthread(int m) {
	uintptr_t v = (uint32_t)(-m);
	pthread_setspecific(G_NODE.handle_key, (void *)v);
}

// 获得当前上下文的句柄值
uint32_t
uboss_current_handle(void) {
	if (G_NODE.init) {
		void * handle = pthread_getspecific(G_NODE.handle_key);
		return (uint32_t)(uintptr_t)handle;
	} else {
		uint32_t v = (uint32_t)(-THREAD_MAIN);
		return v;
	}
}

// 是否为远程
int
uboss_isremote(struct uboss_context * ctx, uint32_t handle, int * harbor) {
	int ret = 0;
// TODO: 屏蔽远程消息检测
//	int ret = uboss_harbor_message_isremote(handle);
	if (harbor) {
		*harbor = (int)(handle >> HANDLE_REMOTE_SHIFT);
	}
	return ret;
}

// 过滤参数
static void
_filter_args(struct uboss_context * context, int type, int *session, void ** data, size_t * sz) {
	int needcopy = !(type & PTYPE_TAG_DONTCOPY);
	int allocsession = type & PTYPE_TAG_ALLOCSESSION;
	type &= 0xff;

	// 允许会话
	if (allocsession) {
		assert(*session == 0);
		*session = uboss_context_newsession(context); // 新建会话
	}

	//  需要复制数据
	if (needcopy && *data) {
		char * msg = uboss_malloc(*sz+1); // 分配数据的内存空间
		memcpy(msg, *data, *sz); // 复制数据到内存空间
		msg[*sz] = '\0'; // 设置新数据的最后为0
		*data = msg; // 将新数据的地址复制给原来数据地址
	}

	// 重建数据长度的值
	*sz |= (size_t)type << MESSAGE_TYPE_SHIFT;
}

// 发送消息
int
uboss_send(struct uboss_context * context, uint32_t source, uint32_t destination , int type, int session, void * data, size_t sz) {
	if ((sz & MESSAGE_TYPE_MASK) != sz) {
		uboss_error(context, "The message to %x is too large", destination);
		if (type & PTYPE_TAG_DONTCOPY) {
			uboss_free(data); // 释放数据的内存空间
		}
		return -1;
	}
	_filter_args(context, type, &session, (void **)&data, &sz); // 过滤参数

	// 如果来源地址为0，表示服务自己
	if (source == 0) {
		source = context->handle;
	}

	// 如果目的地址为0，返回会话ID
	if (destination == 0) {
		return session;
	}
//TODO:屏蔽和远程消息有关代码
//	if (uboss_harbor_message_isremote(destination)) {
//		struct remote_message * rmsg = uboss_malloc(sizeof(*rmsg)); // 分配远程消息的内存空间
//		rmsg->destination.handle = destination; // 目的地址
//		rmsg->message = data; // 数据的地址
//		rmsg->sz = sz; // 数据的长度
//		uboss_harbor_send(rmsg, source, session); // 发送消息到集群中
//	} else { // 本地消息
		struct uboss_message smsg;
		smsg.source = source;
		smsg.session = session;
		smsg.data = data;
		smsg.sz = sz;

		// 将消息压入消息队列
		if (uboss_context_push(destination, &smsg)) {
			uboss_free(data);
			return -1;
		}
//	}
	return session;
}


// 注册服务模块中的返回函数
void
uboss_callback(struct uboss_context * context, void *ud, uboss_cb cb) {
	context->cb = cb; // 返回函数的指针
	context->cb_ud = ud; // 用户的数据指针
}

////////////////////////////////////////////
// 如果uboss上下文总是为0，则终止。
#define CHECK_ABORT if (uboss_context_total()==0) break;

// 以服务名字来发送消息
int
uboss_sendname(struct uboss_context * context, uint32_t source, const char * addr , int type, int session, void * data, size_t sz) {
	if (source == 0) {
		source = context->handle;
	}
	uint32_t des = 0;
	if (addr[0] == ':') {
		des = strtoul(addr+1, NULL, 16);
	} else if (addr[0] == '.') {
		des = uboss_handle_findname(addr + 1);
		if (des == 0) {
			if (type & PTYPE_TAG_DONTCOPY) {
				uboss_free(data);
			}
			return -1;
		}
//TODO:屏蔽和远程消息有管代码
//	} else {
//		_filter_args(context, type, &session, (void **)&data, &sz);

//		struct remote_message * rmsg = uboss_malloc(sizeof(*rmsg));
//		copy_name(rmsg->destination.name, addr);
//		rmsg->destination.handle = 0;
//		rmsg->message = data;
//		rmsg->sz = sz;

//		uboss_harbor_send(rmsg, source, session);
//		return session;
	}

	return uboss_send(context, source, des, type, session, data, sz);
}



// 根据名字请求
uint32_t
uboss_queryname(struct uboss_context * context, const char * name) {
	switch(name[0]) { // 取出第一个字符
	case ':': // 冒号，为数字型名称，需要操作后返回
		return strtoul(name+1,NULL,16);
	case '.': // 点，为字符串名称，直接返回
		return uboss_handle_findname(name + 1);
	}
	uboss_error(context, "Don't support query global name %s",name);
	return 0;
}
/*
 * 重新封装的 创建线程函数
 * */
static void
create_thread(pthread_t *thread, void *(*start_routine) (void *), void *arg) {
	if (pthread_create(thread,NULL, start_routine, arg)) {
		fprintf(stderr, "Create thread failed");
		exit(1); // 退出
	}
}

// 监视器的结构
struct monitor {
	int count; // 总数
	struct uboss_monitor ** m; // uBoss监视器的结构
	pthread_cond_t cond; // 线程条件变量
	pthread_mutex_t mutex; // 线程互斥锁
	int sleep; // 休眠标志
	int quit; // 退出标志
};

// 工作线程的参数
struct worker_parm {
	struct monitor *m; // 监视器的结构
	int id; // 编号
	int weight; // 权重
};

/*
 * 消息调度的工作线程
 * */
static void *
thread_worker(void *p) {
	// 创建工作线程的参数结构体，并获得参数
	struct worker_parm *wp = p;
	int id = wp->id;
	int weight = wp->weight;
	struct monitor *m = wp->m;
	struct uboss_monitor *sm = m->m[id];

	uboss_initthread(THREAD_WORKER); // 初始化线程
	struct message_queue * q = NULL; // 传入NULL消息队列，便于从全局队列中弹出一个消息队列
	while (!m->quit) {
		q = uboss_context_message_dispatch(sm, q, weight); // 核心功能: 从消息队列中取出服务的消息

		// 如果全局消息队列中没有任何消息
		if (q == NULL) {
			if (pthread_mutex_lock(&m->mutex) == 0) { // 设置互斥锁
				++ m->sleep;
				// 伪造 wakeup 是无害的，因为uboss_context_message_dispatch() 可以在任意时间调用
				if (!m->quit)
					pthread_cond_wait(&m->cond, &m->mutex); // 无条件等待
				-- m->sleep;
				if (pthread_mutex_unlock(&m->mutex)) { // 解开互斥锁
					fprintf(stderr, "unlock mutex error");
					exit(1);
				}
			}
		}
	}
	return NULL;
}


/*
 * 唤醒线程
 * */
static void
wakeup(struct monitor *m, int busy) {
	if (m->sleep >= m->count - busy) {
		// signal sleep worker, "spurious wakeup" is harmless
		pthread_cond_signal(&m->cond); // 激发等待 &m->cond 条件的线程
	}
}

/*
 * 定时器工作线程
 * */
static void *
thread_timer(void *p) {
	struct monitor * m = p;
	uboss_initthread(THREAD_TIMER); // 初始化线程
	for (;;) {
		uboss_updatetime(); // 更新事件
		CHECK_ABORT
		wakeup(m,m->count-1); // 唤醒
		usleep(2500); // 睡眠 1/4 秒
	}

	// 唤醒所有消息调度工作线程
	pthread_mutex_lock(&m->mutex); // 设置互斥锁
	m->quit = 1; // 设置退出标志
	pthread_cond_broadcast(&m->cond); // 激发所有线程
	pthread_mutex_unlock(&m->mutex); // 解开互斥锁
	return NULL;
}

/*
 * 释放监视器结构
 * */
static void
free_monitor(struct monitor *m) {
	int i;
	int n = m->count;
	for (i=0;i<n;i++) {
		uboss_monitor_delete(m->m[i]); // 删除监视器
	}
	pthread_mutex_destroy(&m->mutex);
	pthread_cond_destroy(&m->cond);
	uboss_free(m->m); // 释放内存空间
	uboss_free(m); // 释放内存空间
}

/*
 * 监视器工作线程
 * */
static void *
thread_monitor(void *p) {
	struct monitor * m = p;
	int i;
	int n = m->count;
	uboss_initthread(THREAD_MONITOR); // 初始化线程
	for (;;) {
		CHECK_ABORT
		for (i=0;i<n;i++) {
			uboss_monitor_check(m->m[i]);  // 监视器 检查版本
		}

		// 循环 5 次，每次睡眠 1秒
		for (i=0;i<5;i++) {
			CHECK_ABORT
			sleep(1); // 睡眠1秒钟
		}
	}

	return NULL;
}

/*
 * 启动线程
 * */
static void
start_thread(int thread) {
	pthread_t pid[thread+2];

	// 创建线程监视器结构，并初始化数据
	struct monitor *m = uboss_malloc(sizeof(*m));
	memset(m, 0, sizeof(*m));
	m->count = thread; // 线程数量
	m->sleep = 0; // 休眠标志
	m->m = uboss_malloc(thread * sizeof(struct uboss_monitor *)); // 分配监视器结构的内存空间

	// 为每一个线程创建一个监视器
	int i;
	for (i=0;i<thread;i++) {
		m->m[i] = uboss_monitor_new();
	}

	// 初始化线程互斥锁
	if (pthread_mutex_init(&m->mutex, NULL)) {
		fprintf(stderr, "Init mutex error");
		exit(1);
	}

	// 初始化线程条件变量
	if (pthread_cond_init(&m->cond, NULL)) {
		fprintf(stderr, "Init cond error");
		exit(1);
	}

	// 创建监视器线程 Monitor
	create_thread(&pid[0], thread_monitor, m);

	// 创建定时器线程 Timer
	create_thread(&pid[1], thread_timer, m);

	// 权重
	static int weight[] = {
		-1, -1, -1, -1, 0, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1,
		2, 2, 2, 2, 2, 2, 2, 2,
		3, 3, 3, 3, 3, 3, 3, 3,
	};

	// 循环启动 消息调度工作线程
	struct worker_parm wp[thread];
	for (i=0;i<thread;i++) {
		wp[i].m = m;
		wp[i].id = i;
		if (i < sizeof(weight)/sizeof(weight[0])) {
			wp[i].weight= weight[i];
		} else {
			wp[i].weight = 0;
		}
		create_thread(&pid[i+2], thread_worker, &wp[i]); // 创建 工作 线程
	}

	// 循环等待所有线程的结束
	for (i=0;i<thread+2;i++) {
		pthread_join(pid[i], NULL);
	}

	// 释放监视器结构的数据
	free_monitor(m);
}

/*
 * 加载 LUA 引导程序
 * */
static void
bootstrap(struct uboss_context * logger, const char * cmdline) {
	int sz = strlen(cmdline); // 计算字符串长度
	char name[sz+1]; // 服务的名字
	char args[sz+1]; // 服务的参数

	// 将 cmdline 分解为 name 和 args
	sscanf(cmdline, "%s %s", name, args);

	// 新建 uBoss 上下文
	struct uboss_context *ctx = uboss_context_new(name, args);
	if (ctx == NULL) {
		uboss_error(NULL, "Bootstrap error : %s\n", cmdline);
		uboss_context_dispatchall(logger);
		exit(1);
	}
}

/*
 * 启动 uboss 的服务端
 * */
void
uboss_server(struct uboss_config * config) {
	uboss_handle_init(config->harbor); // 初始化 服务句柄模块
	uboss_mq_init(); // 初始化 消息队列模块
	uboss_module_init(config->path_module); // 初始化 服务加载模块
	uboss_timer_init(); // 初始化 定时器模块

	// 创建新的 uBoss 上下文，并加载日志记录器模块
	struct uboss_context *ctx = uboss_context_new(config->log_service, config->log_param);
	if (ctx == NULL) {
		fprintf(stderr, "Can't launch %s service\n", config->log_service);
		exit(1);
	}

	// 加载 lua 引导程序
	bootstrap(ctx, config->bootstrap);

	// 启动线程
	start_thread(config->thread);
}
