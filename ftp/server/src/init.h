#ifndef INIT_H
#define INIT_H
/*
    初始的函数，先初始化服务器，然后与客户端建立连接（包括两个信道）。
*/


#include "globalData.h"
#include "serverCommand.h"
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <arpa/inet.h>

//处理传入的参数，根据参数修改端口，文件夹等信息
bool initArgs(int arg, char** argv);

//初始化服务器，建立监听端口
bool startServe();

//与某个客户端建立连接
int startConnectFd();

//与该用户端进行各种交互的函数入口
void* communication(void* );

# endif