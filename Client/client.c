//
//  Dang Van Do
//  17025077

//  server.c
//  connects to server when starting

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>       // basic system data types
#include <sys/socket.h>      // basic socket definitions
#include <netinet/in.h>      // sockaddr_in{} and other Internet defns
#include <arpa/inet.h>       // inet(3) functions
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#define SERVER_PORT 9999
#define BUFFER_SIZE 1024

void *run(void* arg){//wait for msg from server
    int connSock = *((int*) arg);
    int recvDataLen;
    char recvBuf[BUFFER_SIZE] = {0};
    char cmd[100] = {0};
    char sender[100] = {0};
    char receiver[100] = {0};
    while(1){
        recvDataLen = read(connSock, recvBuf, BUFFER_SIZE);
        if (recvDataLen < 0){
            printf("Error when receiving data\n");
            break;
        }
        recvBuf[recvDataLen] = '\0';
        sscanf(recvBuf, "%s", cmd);
        if(strcmp(cmd, "c") == 0){
            char username[100] = {0};
            sscanf(recvBuf, "%s%s", cmd, username);
            printf("%s connected\n", username);
        }
        else if(strcmp(cmd, "q") == 0){
            char username[100] = {0};
            sscanf(recvBuf, "%s%s", cmd, username);
            printf("%s disconnected\n", username);
        }
        else if(cmd[1] ==  'f'){
            char filename[100] = {0};
            int size = 0;
            sscanf(recvBuf, "%s%s%s%s%d", cmd, sender, receiver, filename, &size);      
            int ctx_idx = strlen(cmd) + strlen(sender) + strlen(receiver) + 3;
            printf("%s %d\n", filename, size);
            FILE *f = fopen(filename, "wb");
            while(size > 0){
                recvDataLen = read(connSock, recvBuf, sizeof(recvBuf));
                printf("%d\n", size);
                if (recvDataLen < 0){
                    printf("Error when sending data\n");
                    break;
                }
                else {
                    
                    fwrite(recvBuf, recvDataLen, 1, f);
                }
                size -= recvDataLen;
            }
            fclose(f);
                printf("file received!!!\n");
        }
        else {
            char ctx[BUFFER_SIZE] = {0};
            sscanf(recvBuf, "%s%s%s", cmd, sender, receiver);
            int idx = strlen(cmd) + strlen(sender) + strlen(receiver) + 3;
            printf("msg from %s: %s\n", sender, &recvBuf[idx]);
        }
    }
    close(connSock);
    return NULL;
}

int main(int argc, char **argv){
    char serverAddress[100];
    int  serverPort;
    int     connSock;
    struct  sockaddr_in servAddr;
    char sentBuf[BUFFER_SIZE], recvBuf[BUFFER_SIZE];
    int  sentDataLen, recvDataLen;
    char args[100] = {0};
    char msg[100] = {0};
    
    if (argc >= 3){
        strcpy(serverAddress,argv[1]);
        serverPort = atoi(argv[2]);
    }
    else{
        strcpy(serverAddress,"127.0.0.1");
        serverPort = SERVER_PORT;
    }
    
    connSock = socket(AF_INET, SOCK_STREAM, 0);
    if (connSock < 0){
        printf("Error when creating socket\n");
        exit(1);
    }
    
    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family      = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(serverAddress);
    servAddr.sin_port        = htons(serverPort);
    if (connect(connSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0){
        printf("Error when connecting!");
        exit(1);
    }
    printf("Connected to the server ...\nPlease enter your name:");

    int user_count = -1;
    while(user_count < 0){//try to register
        scanf("%[^\n]%*c", sentBuf);
        sprintf(msg, "c%s", sentBuf);
        sentDataLen = write(connSock, msg, strlen(msg));
        if (sentDataLen < 0){
            printf("Error when sending data\n");
            exit(1);
        }
        recvDataLen = read(connSock, &user_count, sizeof(int));
        if (recvDataLen < 0){
            printf("Error when receiving data\n");
            exit(1);
        }
        if(user_count < 0){
            printf("User already exists!!!\nRe-enter your user name:");
        }
    }
    printf("Current users:\n");
    while(user_count > 0){
        recvDataLen = read(connSock, recvBuf, BUFFER_SIZE);
        recvBuf[recvDataLen] = '\0';
        printf("%s", recvBuf);
        for(int i = 0; i < recvDataLen; i++){
            if(recvBuf[i] == '\n'){
                user_count -= 1;
            }
        }
    }
    printf("You are connected!!!\nType @EXIT to quit, @<username> <msg> to send msg to another user, #<msg> to broadcast your msg\n");
    pthread_t tid; 
    pthread_create(&tid, NULL, &run, (void *) &connSock);
    char cmd[100] = {0};
    char sender[100] = {0};
    char recv[100] = {0};
    char ctx[BUFFER_SIZE] = {0};

    while(1){//waiting for user input
        scanf("%[^\n]%*c", sentBuf);
        if (strcmp(sentBuf,"@EXIT") == 0){
            sentBuf[0] = 'q';
            send(connSock, sentBuf, 1, 0);
            break;
        }
        else if(sentBuf[0] == '@') {
            sscanf(sentBuf, "%s%s%s%s", cmd, sender, recv, ctx);
            if(cmd[1] == 'f'){
                int count = 0;
                struct stat obj;
                stat(ctx, &obj);
                int size = obj.st_size;
                if (size > 0){
                    sprintf(sentBuf, "%s %s %s %s %d", cmd, sender, recv, ctx, size);
                    send(connSock, sentBuf, strlen(sentBuf), 0);
                    int bytes_read = 0;
                    FILE *f = fopen(ctx, "rb");
                    while(!feof(f)){
                        bytes_read = fread(&sentBuf, 1, BUFFER_SIZE, f);
                        count +=  bytes_read;
                        printf("%d\n", count);
                        if(bytes_read < 0){
                            break;
                        }
                        send(connSock, sentBuf, bytes_read, 0);
                    }
                    fclose(f);
                }
                printf("file sent!!!\n");

            }
            else if(cmd[1] == 'm'){
                send(connSock, sentBuf, strlen(sentBuf), 0); 
            }
        }
    }

    return 1;
}
