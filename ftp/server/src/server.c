/*
*********************************************
                   _ooOoo_
                  o8888888o
                  88" . "88
                  (| -_- |)
                  O\  =  /O
               ____/`---'\____
             .'  \\|     |//  `.
            /  \\|||  :  |||//  \
           /  _||||| -:- |||||-  \
           |   | \\\  -  /// |   |
           | \_|  ''\---/''  |   |
           \  .-\__  `-`  ___/-. /
         ___`. .'  /--.--\  `. . __
      ."" '&lt;  `.___\_&lt;|>_/___.'  >'"".
     | | :  `- \`.;`\ _ /`;.`/ - ` : | |
     \  \ `-.   \_ __\ /__ _/   .-` /  /
======`-.____`-.___\_____/___.-`____.-'======
                   `=---='
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
           佛祖保佑       永无BUG
*/

/*
    *****************此文件是主文件*****************************
*/

#include "init.h"
#include <pthread.h>

int main(int argc, char **argv) {	

    if (initArgs(argc, argv) == false){
        printf("参数传入有误\n");
        return 1;
    }
	
	if (startServe() == false){
        printf("初始化服务器失败\n");
        return 1;
    }

	while (1) {
        int connectfd;
        while (1){
            if ((connectfd = accept(initListenFd, NULL, NULL)) == -1) {
                printf("Error accept(): %s(%d)\n", strerror(errno), errno);
                continue;
            }
            else{
                break;
            }
        }
        //communication((void*)&connectfd);
        pthread_t linkThread;
        pthread_create(&linkThread, NULL, communication, (void*)&connectfd);
		pthread_detach(linkThread);

	}

	close(initListenFd);
    return 0;
}

