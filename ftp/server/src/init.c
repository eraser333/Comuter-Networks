#include "init.h"

bool initArgs(int arg, char** argv){
    //什么参数也没有，用指定的/tmp文件夹
    transPort = 21;
    if (arg == 1){
        strcpy(fileFolder, "/tmp");
    }
    //传入一个参数，需要修改端口或者文件夹
    else if (arg == 3){
        if(strcmp(argv[1],"-port") == 0){
            transPort = atoi(argv[2]);
            strcpy(fileFolder, "/tmp");
        }
        else if(strcmp(argv[1],"-root") == 0) {
            strcpy(fileFolder, argv[2]);
        }
        else{ //传入莫名其妙的参数，报错
            return false;
        }
    }
    //传入两个参数，同时修改文件夹和端口
    else if (arg == 5){
        if(strcmp(argv[1],"-port") == 0 && strcmp(argv[3],"-root") == 0){
            transPort = atoi(argv[2]);
            strcpy(fileFolder, argv[4]);
        }
        else if(strcmp(argv[1],"-root") == 0 && strcmp(argv[3],"-port") == 0) {
            transPort = atoi(argv[4]);
            strcpy(fileFolder, argv[2]);
        }
        else{ //传入莫名其妙的参数，报错
            return false;
        }
    }
    return true;
}

bool startServe(){
    struct sockaddr_in addr;
    //创建socket
	if ((initListenFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return false;
	}

	//设置本机的ip和port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
    //addr.sin_port = htons(transPort);
    addr.sin_port = htons(transPort);
    //addr.sin_addr.s_addr = htonl(INADDR_ANY);	//监听"0.0.0.0"
	
    if (inet_pton(AF_INET, IP, &addr.sin_addr) <= 0) {			
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return false;
	}
    
	//将本机的ip和port与socket绑定
	if (bind(initListenFd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return false;
	}

	//开始监听socket
	if (listen(initListenFd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return false;
	}
    return true;
}

int startConnectFd(){
    int connectfd;
    while (1){
        if ((connectfd = accept(initListenFd, NULL, NULL)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            continue;
        }
        else{
            return connectfd;
        }
    }
    return connectfd;
}

void* communication(void* connectfd){
    //初始化每个客户端的连接
    clientData* clientdata = (clientData*)malloc(sizeof(clientData));
    clientdata->connectType = CONNECTTYPENORMAL;
    clientdata->connectFd = *((int*)connectfd);
    clientdata->listenFd = -1;
    clientdata->dataFd = -1;
    clientdata->listenPort = -1;
    memset(clientdata->PORTIP, 0, 50);
    strcpy(clientdata->filePos, fileFolder);
    //第一步，用户需要先登录
    login(clientdata);
    //第二步，处理其他的指令
    otherCommand(clientdata);
    return NULL;
}