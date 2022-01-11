# ifndef GLOBALDATA_H
# define GLOBALDATA_H

/*
    主要包括各种全项目的全局变量，新定义的类型等等
*/


//*********************************初始化时需要的变量*******************

extern int initListenFd; //初始的监听信道
extern char localIP[20]; //本机的IP地址
extern char IP[20];
extern int transPort;    //端口号
extern char fileFolder[150]; //操作的文件夹号

//*********************************判断指令类型，与每个服客户端通信需要的内容******************

#define CONNECTTYPENORMAL 0
#define CONNECTTYPEPORT   1
#define CONNECTTYPEPASV   2

typedef struct _clientData{
    int connectType;
    int connectFd;
    int listenFd;
    int dataFd;

    int listenPort; //PASV

    int PORTPort;   //PORT
    char PORTIP[50];//PORT

    char filePos[200]; // 文件位置
}clientData;


//******************************发送信息时指定message的最大长度**************************

#define MAX_REQUEST_LEN  200  //client最大的指令长度
#define MAX_RESPONSE_LEN 200  //
#define MAX_FILEPATH_LEN 150
# endif