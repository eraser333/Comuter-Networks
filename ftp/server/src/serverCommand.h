#ifndef SERVERCOMMAND_H
#define SERVERCOMMAND_H

/*
    本文件主要用于处理包括登录以其其他指令，
*/

#include "globalData.h"
#include "prepareInstruction.h"
#include "fileRelated.h"
#include <sys/socket.h>
#include <netinet/in.h>

#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

//*********************************主要功能*******************************************
void login(clientData* clientdata);

void otherCommand(clientData* clientdata); //除登录以外的其他功能

//port命令
void port(clientData* clientdata, char* spcificRequest);

//PASV命令
void pasv(clientData* clientdata);

//SYST命令
void syst(clientData* clientdata);

//TYPE命令
void type(clientData* clientdata, char* specificRequest);

//ABOR命令
void aborOrQuit(clientData* clientdata);

//RETR
void retrOrStor(clientData* clientdata, char* specificRequest, void(*func)(void*));


void cwd(clientData* clientdata, char* specificRequest);

void pwd(clientData* clientdata, char* specificRequest);

void mkd(clientData* clientdata, char* specificRequest);

void list(clientData* clientdata, char* specificRequest);

void rnfr(clientData* clientdata, char* specificRequest);

void rnto(clientData* clientdata, char* specificRequest);

void rmd(clientData* clientdata, char* specificRequest);

void cdup(clientData* clientdata, char* specificRequest);

void failure(clientData* c);


# endif 