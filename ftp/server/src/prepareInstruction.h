#ifndef PREPAREINSTRUCTION_H
#define PREPAREINSTRUCTION_H

#include "globalData.h"
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>

#include<arpa/inet.h>
//**************************************指令准备函数**********************************
//读入,写入用户的消息
bool getMessage(int connectFd, char* message);

bool sendMessage(int connectFd, char* response);

//*************************************判断指令的类型*********************************


// 请求类型的枚举
typedef enum _RequestType {
	// 需求功能
	USER, //0
	PASS, //1
	RETR, //2
	STOR, //3
	QUIT, //4
	SYST, //5
	TYPE, //6
	PORT, //7
	PASV, //8
	MKD,  //9
	CWD,  //10
	PWD,  //11
	LIST, //12
	RMD,  //13
	RNFR, //14
	RNTO, //15
    ABOR, //16
    CDUP, //17
	_FL	// 表示不是合法的请求
}requestTypeEnum;

# define REQUEST_NUM_TYPR 19

typedef struct RequestDecompose{
    requestTypeEnum type;
    char specific[100];
}requestDecompose;


requestDecompose decompose(char* request);

//***********************PORT指令需要的函数***************************
int transStrToInt(char* str);
void getClientIPAndPort(clientData* c, char* specificRequest);

//PORT 和 PASV共有的关闭
void closeFd (clientData* c);

//**********************RETR STOR************************************
//两个指令共有的，开启一个文件传输通道dataFd
bool startDataFd(clientData* c);
bool PORTConnect(clientData* c);
bool PASVConnect(clientData* c);

//共有，变回普通的连接
void returnNormal(clientData* c);

# endif