#include "serverCommand.h"

void login(clientData* clientdata){
    int connfd = clientdata->connectFd;
    //步骤1，服务器向客户端做出准备好的响应
    char response_step1[MAX_RESPONSE_LEN] = "220 Anonymous FTP server ready.\r\n";
    sendMessage(connfd , response_step1);

    //步骤2，准备接收 USER anonymous
    char request_step2[MAX_REQUEST_LEN] = "\0";
    char expect[MAX_REQUEST_LEN] = "anonymous";
    getMessage(connfd , request_step2);
    requestDecompose data_step2 = decompose(request_step2);

    while(strcmp(data_step2.specific, expect) != 0 || data_step2.type != USER){
        char response_error[MAX_RESPONSE_LEN] = "530 Need an anonymous USER!\r\n";
        sendMessage(connfd , response_error);
        memset(request_step2, 0, MAX_REQUEST_LEN);
        getMessage(connfd , request_step2);
        data_step2 = decompose(request_step2);
    }
    
    //步骤3，ask for an email address as a password.
    char response_step3[MAX_RESPONSE_LEN] = "331 Guest login ok, send your e-mail address as password.\r\n";
    sendMessage(connfd, response_step3);

    //步骤4，客户端respond with a “PASS some_password” string.
    char request_step4[MAX_REQUEST_LEN] = "\0";
    getMessage(connfd , request_step4);
    requestDecompose data_step4 = decompose(request_step4);


    while (data_step4.type !=  PASS){
       char response_error[MAX_RESPONSE_LEN] = "530 Need Password!\r\n";
       sendMessage(connfd, response_error);
       memset(request_step4, 0, MAX_REQUEST_LEN);
       getMessage(connfd , request_step4);
       data_step4 = decompose(request_step4);
    }
    
    //步骤5，服务器发送问候
    char response_step5[MAX_RESPONSE_LEN] = "230 Guest login ok, access restrictions apply.\r\n";
    sendMessage(connfd, response_step5);
}


void otherCommand(clientData* clientdata){
    //执行其他命令
    int connfd = clientdata->connectFd;
    char clientRequest[MAX_REQUEST_LEN];
    while(1){
        memset(clientRequest, 0, MAX_REQUEST_LEN);
        getMessage(connfd, clientRequest);

        requestDecompose data = decompose(clientRequest);
        switch(data.type){
            case PORT:
                port(clientdata, data.specific);
                break;
            case PASV:
                pasv(clientdata);
                break;
            case SYST:
                syst(clientdata);
                break;
            case TYPE:
                type(clientdata, data.specific);
                break;
            case ABOR:
            case QUIT:
                aborOrQuit(clientdata);
                break;
            case RETR:
                retrOrStor(clientdata, data.specific, transferStationSend);
                break;
            case STOR:
                retrOrStor(clientdata, data.specific, transferStationStore);
                break;
            case CWD:
                cwd(clientdata, data.specific);
                break;
            case PWD:
                pwd(clientdata, data.specific);
                break;
            case MKD:
                mkd(clientdata, data.specific);
                break;
            case LIST:
                list(clientdata, data.specific);
                break;
            case RNFR: //之后必须是RNTO
                rnfr(clientdata, data.specific);
                break;
            case RMD:
                rmd(clientdata, data.specific);
                break;
            case CDUP:
                cdup(clientdata, data.specific);
                break;
            default: // _FL，失败的情况
                failure(clientdata);
                return;
        }
    }
    free(clientdata);
} 

void port(clientData* clientdata, char* spcificRequest){
    getClientIPAndPort(clientdata, spcificRequest);
    clientdata->connectType = CONNECTTYPEPORT;
    closeFd(clientdata);
    char response[MAX_RESPONSE_LEN] = "200 PORT command successful.\r\n";
    sendMessage(clientdata->connectFd, response);
}

void pasv(clientData* clientdata){
    //先关闭之前的管道
    closeFd(clientdata);
    
    clientdata->connectType = CONNECTTYPEPASV;
    clientdata->listenPort = rand() % 45536 + 20000;

    struct sockaddr_in addr;
    //创建socket
	if ((clientdata->listenFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return;
	}

	//设置本机的ip和port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;

	addr.sin_port = htons(clientdata->listenPort);
    //addr.sin_addr.s_addr = htonl(INADDR_ANY);	//监听"0.0.0.0"
    if (inet_pton(AF_INET, IP, &addr.sin_addr) <= 0) {			
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return;
	}
	//将本机的ip和port与socket绑定
	while (bind(clientdata->listenFd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d), changing port\n", strerror(errno), errno);
		clientdata->listenPort = rand() % 45536 + 20000;
		addr.sin_port = htons(clientdata -> listenPort);
		//return;
	}
	//开始监听socket
	if (listen(clientdata->listenFd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return;
	}

    //**************************返回响应*******************************
    char response[MAX_RESPONSE_LEN];

    int port_first = clientdata->listenPort / 256; 
    int port_second = clientdata->listenPort % 256;

    sprintf(response, "227 Entering Passive Mode (%s,%d,%d)\r\n", localIP, port_first, port_second);
    sendMessage(clientdata->connectFd, response);
}


void syst(clientData* clientdata){
    char response[MAX_RESPONSE_LEN] = "215 UNIX Type: L8\r\n";
    sendMessage(clientdata->connectFd, response);
}

void type(clientData* clientdata, char* specificRequest){
    char standord[2] = "I";
    if (strcmp(standord, specificRequest) == 0){
        char success[MAX_RESPONSE_LEN] = "200 Type set to I.\r\n";
        sendMessage(clientdata->connectFd, success);
    }
    else{
        char error[MAX_RESPONSE_LEN] = "435 Illegal Type\r\n";
        sendMessage(clientdata->connectFd, error);
    }
}

void aborOrQuit(clientData* clientdata){
    char goodbye[MAX_RESPONSE_LEN] = "221 Goodbye.\r\n";
    sendMessage(clientdata -> connectFd, goodbye);
}

void retrOrStor(clientData* clientdata, char* specificRequest, void(*func)(void*)){
    //如果是合适的连接方式 port or pasv
    if (clientdata->connectType == CONNECTTYPEPORT || clientdata->connectType == CONNECTTYPEPASV){
        //先回复准备开始传送文件
        char startTransport[MAX_RESPONSE_LEN] =  "150 Transport Start.\r\n";
        sendMessage(clientdata->connectFd, startTransport);

        //获取连接
        if(startDataFd(clientdata)){
            pthread_t linkThread;
            threadData t;
            t.c = clientdata;
            t.specific = specificRequest;
            pthread_create(&linkThread, NULL, (void*)func, (void*)&t);
            pthread_detach(linkThread);
            //变回普通的连接方式
            returnNormal(clientdata);
        }
        //获取连接失败
        else{
            char errorResponse[MAX_RESPONSE_LEN] = "426 PORT Connection Fail.\r\n";
            sendMessage(clientdata->connectFd, errorResponse);
        }
    }
    //如果不是，直接报错
    else{
        char response[MAX_RESPONSE_LEN] = "425 No Appropriate Connection.\r\n";
        sendMessage(clientdata->connectFd, response);
    }
}

void cwd(clientData* clientdata, char* specificRequest){
    if (changeWorkingDirectory(clientdata, specificRequest) == false){
        char fail[MAX_RESPONSE_LEN] =  "550 Change Directory Error.\r\n";
        sendMessage(clientdata->connectFd, fail);
    }
    else{
        char success[MAX_RESPONSE_LEN] = "250 Change Directory Success.\r\n";
        sendMessage(clientdata->connectFd, success);
    }
}

void pwd(clientData* clientdata, char* specificRequest){
    char message[MAX_FILEPATH_LEN];
    memset(message, 0, MAX_FILEPATH_LEN);

    char response[MAX_RESPONSE_LEN];
    if (printWorkingDirectory( clientdata, message) == false){
        strcpy(response, "550 Change Directory Error.\r\n");
    }
    else{
        sprintf(response, "257 \"%s\" Already Exist.\r\n", message);
    }

    sendMessage(clientdata->connectFd, response);
}


void mkd(clientData* clientdata, char* specificRequest){
    char response[MAX_RESPONSE_LEN];
    if (makeDirectory(clientdata, specificRequest) == false){
        strcpy(response, "550 Change Directory Error.\r\n");
    }
    else{
        strcpy(response, "250 Change Directory success.\r\n");
    }
    sendMessage(clientdata->connectFd, response);
}

void list(clientData* clientdata, char* specificRequest){
    if (clientdata->connectType == CONNECTTYPEPORT || clientdata->connectType == CONNECTTYPEPASV){
        //先回复准备开始传送文件
        char startTransport[MAX_RESPONSE_LEN] =  "150 Transport Start.\r\n";
        sendMessage(clientdata->connectFd, startTransport);

        //获取连接
        if(startDataFd(clientdata)){
            if (sendList(clientdata, specificRequest) == false){
                return;
            }
            char success[MAX_RESPONSE_LEN] = "226 Sending Success.\r\n";
			sendMessage(clientdata->connectFd, success);
            //变回普通的连接方式
            returnNormal(clientdata);
        }
        //获取连接失败
        else{
            char errorResponse[MAX_RESPONSE_LEN] = "426 PORT Connection Fail.\r\n";
            sendMessage(clientdata->connectFd, errorResponse);
        }
    }
    //如果不是，直接报错
    else{
        char response[MAX_RESPONSE_LEN] = "425 No Appropriate Connection.\r\n";
        sendMessage(clientdata->connectFd, response);
    }
}

void rmd (clientData* clientdata, char* specificRequest){
    char response[MAX_RESPONSE_LEN];
    if (removeDirectory(clientdata, specificRequest) == false){
        strcpy(response, "550 Remove Directory Error.\r\n");
    }
    else{
        strcpy(response, "250 Remove Directory Success.\r\n");
    }
    sendMessage(clientdata->connectFd, response);
}

void rnfr(clientData* clientdata, char* specificRequest){
    if (checkDirectory(clientdata, specificRequest) == false){
        char response[MAX_RESPONSE_LEN] = "550 Directory Not Exist.\r\n";
        sendMessage(clientdata->connectFd, response);
    } 
    else{
        char wait[MAX_RESPONSE_LEN] = "350 WAITING FOR RNTO.\r\n";
        sendMessage(clientdata->connectFd, wait);

        while(1){
            char request[MAX_REQUEST_LEN];
            getMessage(clientdata->connectFd, request);
            requestDecompose data = decompose(request);

            if(data.type == RNTO){
                if (renameDirectory(clientdata, specificRequest, data.specific) == false){
                    char response[MAX_RESPONSE_LEN] = "550 Directory Not Exist.\r\n";
                    sendMessage(clientdata->connectFd, response);
                }
                else{
                    char response[MAX_RESPONSE_LEN] = "250 Directory Rename Success.\r\n";
                    sendMessage(clientdata->connectFd, response);
                }
                break;
            }
            else{
                char response[MAX_RESPONSE_LEN] = "500 Must be RNTO after RNFR.\r\n";
                sendMessage(clientdata->connectFd, response);
            }
        }
    }
}


void cdup(clientData* clientdata, char* specificRequest){
    char response[MAX_REQUEST_LEN];
    if (changeToRoot(clientdata) == false){
        strcpy(response, "550 Change Directory Error.\r\n");
    }
    else{
        strcpy(response, "250 Change Directory Success.\r\n");
    }
    sendMessage(clientdata->connectFd, response);
}


void failure(clientData* c){
    char response[MAX_RESPONSE_LEN] = "500 Wrong Command.\r\n";
    sendMessage(c->connectFd, response);
}