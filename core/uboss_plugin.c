/*
** Copyright (c) 2014-2017 uboss.org All rights reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Plug-in
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#include "uboss.h"
#include "uboss_plugin.h"

#include <pthread.h>

#include <assert.h>
#include <string.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

// 插件的最大数量（即类型数量）
#define MAX_PLUGIN_COUNT 32

// 插件的结构
struct plugins {
	int count; // 数量
	struct spinlock lock; // 锁
	const char * path; // 插件的路径
	struct uboss_plugin p[MAX_PLUGIN_COUNT]; // 插件的数组
};

static struct plugins * P = NULL; // 声明插件的结构


void *
uboss_plugin_instance_create(struct uboss_plugin *p) {
	if (p->create) { // 如果创建函数存在
		return p->create(); // 返回调用插件中的创建函数
	} else {
		// C语言中 ~ 符号为按位取反的意思
		// 例如： ~0x37=~(0011 0111)=(1100 1000)=0xC8
		return (void *)(intptr_t)(~0); // 返回空指针 即：0xFFFF FFFF
	}
}

// 这个初始化函数是必须的
int
uboss_plugin_instance_init(struct uboss_plugin *p, void * inst, const char * parm) {
	return p->init(inst, parm); // 返回调用插件中的初始化函数
}


void
uboss_plugin_instance_release(struct uboss_plugin *p, void *inst) {
	if (p->release) { // 如果释放函数存在
		p->release(inst); // 返回调用插件中的释放函数
	}
}

// 检查点函数
void
uboss_plugin_check(const char * name)
{

}

// 注册检查点的返回函数
void
uboss_plugin_register(const char * name, uboss_plugin_cb cb)
{

}

// 初始化插件
void
uboss_plugin_init(const char *path) {
	struct plugins *p = uboss_malloc(sizeof(*p)); // 分配内存空间
	p->count = 0; // 将插件数设置为0
	p->path = uboss_strdup(path); // 设置插件的路径

	SPIN_INIT(p) // 初始化锁

	P = p; // 将插件结构指针赋值给全局变量

	// 循环打开path下的所有插件动态链接库
}

