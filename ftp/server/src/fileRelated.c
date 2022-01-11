# include "fileRelated.h"

void transferStationSend(void* data){
    sendfile(((threadData*)data)->c, ((threadData*)data)->specific);
}

void transferStationStore(void* data){
    storefile(((threadData*)data)->c, ((threadData*)data)->specific);
}


bool getFilePath(char* accurateFilePath, char* folder, char* rawFilePath){
    memset(accurateFilePath, 0, MAX_FILEPATH_LEN);
    if (strlen(rawFilePath)== 0 || rawFilePath[0] != '/'){
        strcpy(accurateFilePath, folder);
        int lenOffileFolder = strlen(folder);
        if (accurateFilePath[lenOffileFolder-1] != '/'){
            accurateFilePath[lenOffileFolder] = '/';
        }
        strcat(accurateFilePath, rawFilePath);
    }
    else{
        if (strlen(rawFilePath) == 1){
            strcpy(accurateFilePath, fileFolder);
        }
        else{
            strcpy(accurateFilePath, fileFolder);
            strcat(accurateFilePath, rawFilePath);
        }
    }
    return true;
}


bool sendfile(clientData* c, char* specficPos){
    int startPos = 0;
    char accurateFilePath[MAX_FILEPATH_LEN];
    memset(accurateFilePath, 0 , MAX_FILEPATH_LEN);

    if (getFilePath(accurateFilePath, c->filePos, specficPos) == false ){
        char response[MAX_RESPONSE_LEN] = "451 Reading File Error.\r\n";
        sendMessage(c->connectFd, response);
        close(c->dataFd);
        return false;
    }

    FILE *filePtr = fopen(accurateFilePath, "rb");
    if (filePtr == NULL){
        return false;
    }
    
    if (fseek(filePtr, startPos, SEEK_SET) != 0){
        char response[MAX_RESPONSE_LEN] = "451 Can't Start.\r\n";
        sendMessage(c->connectFd, response);
        close(c->dataFd);
        return false;
    }

    //文件传输，8192为一个单位
    char sendFileBuffer[8192];
    memset(sendFileBuffer, 0, 8192);
    while(!feof(filePtr)){
        int p = 0;
        int len = fread(sendFileBuffer, 1, 8192, filePtr);
        while (p < len) {
            int n = write(c->dataFd, sendFileBuffer + p, len - p);
            if (n < 0) {
                printf("Error write(): %s(%d)\n", strerror(errno), errno);
                char response[MAX_RESPONSE_LEN] = "426 Transport Fail.\r\n";
                sendMessage(c->connectFd, response);
                fclose(filePtr);
                return false;
            } 
            else {
                p += n;
            }			
        }
    }
    
    char success[MAX_RESPONSE_LEN] = "226 Sending Success.\r\n";
    sendMessage(c->connectFd, success);
    fclose(filePtr);
    close(c->dataFd);

    return true;
}


bool storefile(clientData* c, char* filename){
    int startPos = 0;
    char accurateFilePath[MAX_FILEPATH_LEN];
    memset(accurateFilePath, 0 , MAX_FILEPATH_LEN);

    //如果文件名不存在或者其他原因导致其长度为0
    if (strlen(filename) == 0 || strlen(c->filePos) == 0){
        char response[MAX_RESPONSE_LEN] = "451 Can't Start.\r\n";
        sendMessage(c->connectFd, response);
        close(c->dataFd);
        return false;
    }
    getFilePath(accurateFilePath, c->filePos, filename);
    FILE* filePtr = fopen(accurateFilePath, "rb+");
    if (filePtr == NULL && startPos == 0){
        filePtr = fopen(accurateFilePath, "wb+");
    }

    if(filePtr == NULL || fseek(filePtr, startPos, SEEK_SET) != 0){
        char error[MAX_RESPONSE_LEN] = "451 Can't Start.\r\n";
        sendMessage(c->connectFd, error);
        close(c->connectFd);
        fclose(filePtr);
        return false;
    }

    //写入文件，同8192一个单位
    char storeFileBuffer[8192];
    memset(storeFileBuffer, 0, 8192);
    while(1){
        int len = read(c->dataFd, storeFileBuffer, 8192);
        //连接断开
        if(len < 0){
            char responseError[MAX_RESPONSE_LEN] = "426 Connection Broken.\r\n";
            sendMessage(c->connectFd, responseError);
            close(c->dataFd);
            fclose(filePtr);
            return false;   
        }
        else if(len == 0){
            break;
        }
        else{
            fwrite(storeFileBuffer, 1, len, filePtr);
        }
    }

    char success[MAX_RESPONSE_LEN] = "226 Receiving Success.\r\n";
    sendMessage(c->connectFd, success);
    fclose(filePtr);
    close(c->dataFd);
    return true;
}





bool printWorkingDirectory(clientData* c, char* message){
    memset(c->filePos, 0 ,MAX_FILEPATH_LEN);
    char* temp = getcwd(NULL, 0);
    strcpy(c->filePos, temp);
    if (strlen(c -> filePos) == strlen(fileFolder)){
        memcpy(message, "/", 1);
    }
    else{
        memcpy(message, c->filePos  +strlen(fileFolder), strlen(c -> filePos) - strlen(fileFolder));
    }

    return true;
} 

bool makeDirectory(clientData* c, char* specific){
    char accurateFilePath[MAX_FILEPATH_LEN];
    memset(accurateFilePath, 0 , MAX_FILEPATH_LEN);

    if (getFilePath(accurateFilePath, c->filePos, specific) == false ){
        return false;
    }

    if (chdir(c->filePos) < 0){
        return false;
    }

    if (mkdir(accurateFilePath, 0) < 0){
        return false;
    }

    return true;
}

bool changeWorkingDirectory(clientData* c, char* specificPos){
    char accurateFilePath[MAX_FILEPATH_LEN];
    memset(accurateFilePath, 0 , MAX_FILEPATH_LEN);

    if (getFilePath(accurateFilePath, c->filePos, specificPos) == false ){
        return false;
    }

    if (chdir(accurateFilePath) < 0){
        return false;
    }

    memset(c->filePos, 0 ,MAX_FILEPATH_LEN);
    char* temp = getcwd(NULL, 0);
    strcpy(c->filePos, temp);

    return true;
}

bool sendList(clientData* c, char* specific){
    if (strlen(specific) != 0){
        if (changeWorkingDirectory(c, specific) == false){
            return false;
        }
    }
    
    
    FILE* filePtr = popen("ls -l ", "r");

    if (filePtr == NULL){
        return false;
    }

    char list[8192];
    memset(list, 0,  8192);
    fread(list, 1, 8192, filePtr);
    pclose(filePtr);

    sendMessage(c->dataFd, list);
    close(c->dataFd);
    return true;
}

bool removeDirectory(clientData* c, char* specific){
    char accurateFilePath[MAX_FILEPATH_LEN];
    memset(accurateFilePath, 0 , MAX_FILEPATH_LEN);

    if (getFilePath(accurateFilePath, c->filePos, specific) == false ){
        return false;
    }

    if(chdir(fileFolder) < 0 || rmdir(accurateFilePath) < 0){
        return false;
    }

    memset(c->filePos, 0, MAX_FILEPATH_LEN);
    char *temp = getcwd(NULL, 0);
    strcpy(c->filePos, temp);
    return true;
}

bool checkDirectory(clientData* c, char* specific){
    char accurateFilePath[MAX_FILEPATH_LEN];
    memset(accurateFilePath, 0 , MAX_FILEPATH_LEN);

    if (getFilePath(accurateFilePath, c->filePos, specific) == false ){
        return false;
    }
    if (access(accurateFilePath, 0) < 0){
        return false;
    }
    else{
        return true;
    }
}

bool renameDirectory(clientData* c, char* old, char* new){
    char oldPath[MAX_FILEPATH_LEN];
    char newPath[MAX_FILEPATH_LEN];
    memset(oldPath, 0,MAX_FILEPATH_LEN);
    memset(newPath, 0, MAX_FILEPATH_LEN);
    if (getFilePath(oldPath, c->filePos, old) == false || getFilePath(newPath, c->filePos, new) == false){
        return false;
    }

    if (chdir(fileFolder) < 0 || rename(oldPath, newPath) < 0){
        return false;
    }
    memset(c->filePos, 0, MAX_FILEPATH_LEN);
    char* temp = getcwd(NULL, 0);
    strcpy(c->filePos, temp);
    return true;
}

bool changeToRoot(clientData* c){
    if (chdir(fileFolder) < 0){
        return false;
    }
    else{
        memset(c->filePos, 0, MAX_FILEPATH_LEN);
        char* temp = getcwd(NULL, 0);
        strcpy(c->filePos, temp);
        return true;
    }
}