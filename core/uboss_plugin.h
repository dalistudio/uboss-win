/*
** Copyright (c) 2014-2017 uboss.org All rights reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Plug-in
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#ifndef UBOSS_PLUGIN_H
#define UBOSS_PLUGIN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// 定义函数指针类型
typedef int (*uboss_plugin_cb)(void *data);

typedef void * (*uboss_plugin_create_dl)(void);
typedef int (*uboss_plugin_init_dl)(void * inst, const char * parm);
typedef void (*uboss_plugin_release_dl)(void * inst);

struct uboss_plugin {
	const char * name; // 模块名称
	void * pulgin; // 模块的地址
	uboss_plugin_create_dl create; // 创建函数的地址
	uboss_plugin_init_dl init; // 初始化函数的地址
	uboss_plugin_release_dl release; // 释放函数的地址
};

struct plugin_storage {
	const char * check_name; // 检查点的名称
	int check_slot; // 检查点的槽数
	int check_count; // 检查点的总数
	int check_weight; // 检查点的权重
};

// 检查点的定义
#define CP_MAIN_START 0
#define CP_MAIN_END 1
#define CP_SERVER_START 2
#define CP_SERVER_END 3
#define CP_BOOTSTRAP_START 4
#define CP_BOOTSTRAP_END 5
#define CP_THREAD_START 6
#define CP_THREAD_END 7
#define CP_MONITER_START 8
#define CP_MONITER_END 9
#define CP_WORKER_START 10
#define CP_WORKER_END 11
#define CP_DISPATCH_START 12
#define CP_DISPATCH_END 13
#define CP_SEND_START 14
#define CP_SEND_END 15

void * uboss_plugin_instance_create(struct uboss_plugin *p);
int uboss_plugin_instance_init(struct uboss_plugin *p, void * inst, const char * parm);
void uboss_plugin_instance_release(struct uboss_plugin *p, void *inst);

void uboss_plugin_check(uint32_t id, void *data);
void uboss_plugin_register(uint32_t id, uboss_plugin_cb cb);
void uboss_plugin_init(const char *path);

#endif /* UBOSS_PLUGIN_H */
