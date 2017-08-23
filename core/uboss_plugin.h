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

struct pplugin_storage {
	const char * check_name; // 检查点的名称
	int check_slot; // 检查点的槽数
	int check_count; // 检查点的总数
	int check_weight; // 检查点的权重
};



void * uboss_plugin_instance_create(struct uboss_plugin *p);
int uboss_plugin_instance_init(struct uboss_plugin *p, void * inst, const char * parm);
void uboss_plugin_instance_release(struct uboss_plugin *p, void *inst);

void uboss_plugin_check(const char * name);
void uboss_plugin_register(const char * name, uboss_plugin_cb cb);
void uboss_plugin_init(const char *path);

#endif /* UBOSS_PLUGIN_H */
