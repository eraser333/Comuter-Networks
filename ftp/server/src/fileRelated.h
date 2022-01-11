# ifndef FILERELATED_H
# define FILERELATED_H
/*
    本文件主要对文件进行上传和下载的RETR和STOR命令进行函数提供
    本来计划该文件应该存在于prepareInstruction文件中
    但考虑到其篇幅和独立性，故单独写一个文件
*/


# include "globalData.h"
# include "prepareInstruction.h"
# include <stdbool.h>
# include <stdio.h>
# include <unistd.h>
# include <sys/stat.h>


//**************************************************
/*
    为实现无阻塞传输，开启多线程，需要将参数封装到结构体
*/
typedef struct ThreadData{
    clientData* c;
    char* specific;
}threadData;

void transferStationSend(void*);

void transferStationStore(void*);

//****************************************************
bool getFilePath(char* accurateFilePath,char* folder, char* rawFilePath);

bool sendfile(clientData* c, char* specificPos);

bool storefile(clientData* c, char* specificPos);

bool changeWorkingDirectory(clientData* c, char* specificPos);

bool printWorkingDirectory(clientData* c, char* message);

bool makeDirectory(clientData* c, char* specific);

bool sendList(clientData* c, char* specific);

bool removeDirectory(clientData* c, char* specific);

bool checkDirectory(clientData* c, char* specific);

bool renameDirectory(clientData* c, char* old, char* new);

bool changeToRoot(clientData* c);
# endif