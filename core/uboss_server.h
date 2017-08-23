/*
** Copyright (c) 2014-2017 uboss.org All rights reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Server
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#ifndef UBOSS_SERVER_H
#define UBOSS_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#define THREAD_WORKER 0
#define THREAD_MAIN 1
#define THREAD_TIMER 2
#define THREAD_MONITOR 3

// 服务器的配置结构 4*8=32字节
struct uboss_config {
	int thread; // 工作线程数
	int harbor; // 集群Id

	const char * path_root; // 根目录的路径
	const char * path_plugin; // 插件的路径
	const char * path_module; // 模块的路径
	const char * path_log; // 本地日志文件的保存路径

	const char * log_service; // 日志服务的名称
	const char * log_param; // 日志服务的参数

	const char * bootstrap; // LUA引导程序的名称
};



struct uboss_context;

int uboss_isremote(struct uboss_context *, uint32_t handle, int * harbor);
uint32_t uboss_current_handle(void);

int uboss_send(struct uboss_context * context, uint32_t source, uint32_t destination , int type, int session, void * msg, size_t sz);
void uboss_server(struct uboss_config * config);

int uboss_sendname(struct uboss_context * context, uint32_t source, const char * addr , int type, int session, void * data, size_t sz);


void uboss_globalinit(void);
void uboss_globalexit(void);
void uboss_initthread(int m);

#endif /* UBOSS_SERVER_H */
