#include "prepareInstruction.h"

const char requestTypeArray[20][5]= {
    {"USER"},
    {"PASS"},
    {"RETR"},
    {"STOR"},
    {"QUIT"},
    {"SYST"},
    {"TYPE"},
    {"PORT"},
    {"PASV"},
    {"MKD "},
    {"CWD"},
    {"PWD"},
    {"LIST"},
    {"RMD "},
    {"RNFR"},
    {"RNTO"},
    {"ABOR"},
    {"CDUP"},
    {"_FL"},// 自定义的错误类型
    {},
};

bool getMessage(int connectFd, char* message){
    int p = 0;
    while (p < MAX_REQUEST_LEN) {
        int n = read(connectFd, message + p, MAX_REQUEST_LEN-p);
        if (n < 0) {
            printf("Error read(): %s(%d)\n", strerror(errno), errno);
            close(connectFd);
            return false;
        } else if (n == 0) {
            break;
        } else {
            p += n;
            if ( message[p - 1] == '\n') {
                break;
            }
        }
    }
    //socket接收到的字符串并不会添加'\0'
    message[p] = '\0';

    return true;
}

bool sendMessage(int connectFd, char* response){
    int p = 0;
    int len = strlen(response);
    while (p < len) {
        int n = write(connectFd, response + p, len - p);
        if (n < 0) {
            printf("Error write(): %s(%d)\n", strerror(errno), errno);
            return false;
        } else {
            p += n;
        }			
    }

    return true;
}


requestDecompose decompose(char* request){
    char prefix[5] = "\0";
    char prefix_pwd[4];
    strncpy(prefix, request, 4);
    strncpy(prefix_pwd, request, 3);
    requestDecompose data;

    //确定指令的类型
    bool flag = false;
    for(int i = 0; i < REQUEST_NUM_TYPR - 1; i++){
        if (i == 10 || i == 11){
            if (strcmp(prefix_pwd, requestTypeArray[i]) == 0){
                data.type = i;
                flag = true;
                break;
            }
        }
        else{
            if (strcmp(prefix, requestTypeArray[i]) == 0){
                data.type = i;
                flag = true;
                break;
            }
        }
        
    }
    //没有找到合适的类型
    if (!flag){
        data.type = _FL;
        return data;
    }

    //确定指令的具体内容

    bool signal = false;
    
    int j = 0;
    memset(data.specific, 0,100);

    for(int i = 3;  i < MAX_REQUEST_LEN && request[i] != '\0' && request[i] != '\r' && request[i] != '\n'; i++){
        if (signal == false){
            if (request[i] == ' '){
                signal = true;
            }
        }
        else if (signal == true){
            if (request[i] != ' '){
                data.specific[j] = request[i];
                j++;
            }
        }
    } 
    data.specific[j] = '\0'; 
    
    return data;
}


int transStrToInt(char* str){
    int value = 0;
    int len = strlen(str);
    for (int i = 0;  i < len; i++){
        value = value * 10 + str[i] - '0';
    }
    return value;
}

void getClientIPAndPort(clientData* c, char* specificRequest){
    int count = 0;
    char temp[10];
    int j = 0;
    int len = strlen(specificRequest);
    int port = 0;
    for(int i = 0; i < len ; i++){
        if (specificRequest[i] != ','){
            temp[j] = specificRequest[i];
            j++;
        }
        else{
            if (count < 3){
                temp[j] = '.';
                temp[++j] = '\0';
                count++;
                j = 0;
                strcat(c->PORTIP, temp);
            }
            else if (count == 3){
                temp[j] = '\0';
                count++;
                j = 0;
                strcat(c->PORTIP, temp);
            }
            else{
                temp[j] = '\0';
                count++;
                j = 0;
                port = transStrToInt(temp);
            }
        }
    }

    temp[j] = '\0';
    port = 256* port + transStrToInt(temp);
    c->PORTPort = port;
}

void closeFd(clientData* clientdata){
    if (clientdata->dataFd != -1){
        close(clientdata->dataFd);
        clientdata->dataFd = -1;
    }
    if (clientdata->listenFd != -1){
        close(clientdata->listenFd);
        clientdata->listenFd = -1;
    }
}

bool startDataFd(clientData* clientdata){
    bool flag = false;
    if (clientdata->connectType == CONNECTTYPEPORT){
        flag = PORTConnect(clientdata); 
    }
    else if (clientdata->connectType == CONNECTTYPEPASV){
        flag = PASVConnect(clientdata);
    }
    return flag;
}

bool PORTConnect(clientData* c){
    struct sockaddr_in addr;
    //创建
    if ((c->dataFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return false;
	}

    //根据之前PORT指令得到的IP进行连接
    //设置本机的ip和port
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(c->PORTPort);

    if (inet_pton(AF_INET, c->PORTIP, &addr.sin_addr) <= 0) {			
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return false;
	}

    if (connect(c -> dataFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		printf("Error connect(): %s(%d)\n", strerror(errno), errno);
		return false;
	}

    return true;
}

bool PASVConnect(clientData* c){
    //直接开启一个文件传输的信道
    if ((c->dataFd = accept(c -> listenFd, NULL, NULL)) == -1){
        return false;
    }
    return true;
}


void returnNormal(clientData* c){
    if (c->listenFd != -1){
        close(c->listenFd);
        c->listenFd = -1;
    }

    c->connectType = CONNECTTYPENORMAL;
}


