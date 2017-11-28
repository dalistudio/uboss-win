/*
** Copyright (c) 2014-2017 uboss.org All rights reserved.
** uBoss - A Lightweight MicroService Framework
**
** uBoss Main Function
**
** Dali Wang<dali@uboss.org>
** See Copyright Notice in uboss.h
*/

#include "uboss.h"
#include "uboss_server.h"
#include "uboss_handle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <pthread.h>

#if !defined(__WIN32__)
// 信号处理，屏蔽 SIGPIPE 信号，避免进程退出
int sigign() {
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);
	return 0;
}
#endif

int
main(int argc, char *argv[]) {

	const char * config_file = NULL ;

	// 执行必须有一个参数，否则打印错误信息。
	if (argc > 1) {
		config_file = argv[1];
	} else {
//		fprintf(stderr, "usage: uboss configfilename\n");
//		return 1;
		config_file = "uboss.conf";
	}

//	fprintf(stdout, "Config File = %s\n", config_file);


	uboss_globalinit(); // 全局初始化

	#if !defined(__WIN32__)
	sigign(); // 信号处理
	#endif

	// 声明 配置文件 的结构
	struct uboss_config config;

	#if !defined(__WIN32__)
	config.path_root = "./"; // 根目录
	config.path_plugin = "./plugin/?.up"; // 插件路径
	config.thread =  8; // 启动工作线程数
	config.path_module = "./module/?.so"; // C写的模块路径
	config.harbor = 1; // 集群的编号
	config.bootstrap = "luavm bootstrap"; // 启动脚本
	config.log_service = "logger"; // 日志记录器的服务
	config.log_param = "uboss.log"; // 日志记录器
	config.path_log = "."; // 保存日志的路径
	#else
	config.path_root = ".\\"; // 根目录/
	config.path_plugin = ".\\plugin\\?.up"; // 插件路径
	config.thread =  8; // 启动工作线程数
	config.path_module = ".\\module\\?.so"; // C写的模块路径
//fprintf(stdout, "Module Path = %s\n", config.path_module);
	config.harbor = 1; // 集群的编号
	config.bootstrap = "luavm bootstrap"; // 启动脚本
	config.log_service = "logger"; // 日志记录器的服务
	config.log_param = "uboss.log"; // 日志记录器
	config.path_log = "."; // 保存日志的路径
	#endif

	uboss_server(&config); // 启动 uBoss 框架
	uboss_globalexit(); // 退出全局初始化


	return 0;
}
