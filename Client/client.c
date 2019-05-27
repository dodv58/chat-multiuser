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
#include "../circular-buffer.h"

#define SERVER_PORT 9999
#define BUFFER_SIZE 1024

void *run(void* arg){//wait for msg from server
    int connSock = *((int*) arg);
    int recvDataLen;
    char recvBuf[BUFFER_SIZE] = {0};
    char cmd[100] = {0};
    char sender[100] = {0};
    char receiver[100] = {0};
    cbuffer_t cbuff;
    cbuffer_init(&cbuff);
    char msg[BUFFER_SIZE] = {0};
    FILE* f;
    int f_size = 0;

    while(1){
        recvDataLen = read(connSock, recvBuf, BUFFER_SIZE);
        if (recvDataLen < 0){
            printf("Error when receiving data\n");
            break;
        }
        cbuffer_insert(&cbuff, recvBuf, recvDataLen);
        int msg_len = cbuffer_get_msg(&cbuff, msg, sizeof(msg));
        int int_size = sizeof(int);
        if(msg_len > 0){
            if(msg[int_size] == '#' && msg[int_size + 1] == 'c'){
                char username[100] = {0};
                sscanf(&msg[int_size + 3], "%s", username);
                printf("%s connected\n", username);
            }
            else if(msg[int_size] == '#' && msg[int_size + 1] == 'q'){
                char username[100] = {0};
                sscanf(&msg[int_size + 3], "%s", username);
                printf("%s disconnected\n", username);
            }
            else if(msg[int_size] == '@' && msg[int_size + 1] == 'm'){
                char ctx[BUFFER_SIZE] = {0};
                sscanf(&msg[int_size + 3], "%s%s%s", sender, receiver, ctx);
                printf("msg from %s: %s\n", sender, ctx);
            }
            else if(msg[int_size] == '@' && msg[int_size + 1] == 'f'){
                char filename[100] = {0};
                sscanf(&msg[int_size + 3], "%s%s%s%d", sender, receiver, filename, &f_size);      
                printf("===> %s %d\n", filename, f_size);
                f = fopen(filename, "wb");
            }
            else if(msg[int_size] == '@' && msg[int_size + 1] == 'd'){
                sscanf(&msg[int_size + 3], "%s%s", sender, receiver);      
                int header_len = int_size + 3 + strlen(sender) + strlen(receiver) + 2;
                fwrite(&msg[header_len], msg_len - header_len, 1, f);
                f_size -= msg_len - header_len;
                if(f_size <= 0){
                    fclose(f);
                    printf("file received!!!\n");
                }
            }
        }
    }
    close(connSock);
    return NULL;
}

int create_msg(char* dest, char* msg){
    int len = strlen(msg) + sizeof(int);
    memmove(dest, &len, sizeof(int));
    memmove(&dest[sizeof(int)], msg, strlen(msg));
    return len;
}

int main(int argc, char **argv){
    char serverAddress[100];
    int  serverPort;
    int     connSock;
    struct  sockaddr_in servAddr;
    char sentBuf[BUFFER_SIZE], recvBuf[BUFFER_SIZE];
    int  sentDataLen, recvDataLen;
    char args[100] = {0};
    char msg[BUFFER_SIZE] = {0};
    
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
    char name[100];
    while(user_count < 0){//try to register
        scanf("%[^\n]%*c", name);
        sprintf(msg, "#c %s #", name);
        int len = create_msg(sentBuf, msg);
        sentDataLen = write(connSock, sentBuf, len);
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
    cbuffer_t buff;
    cbuffer_init(&buff);
    while(user_count > 0){
        recvDataLen = read(connSock, recvBuf, BUFFER_SIZE);
        cbuffer_insert(&buff, recvBuf, recvDataLen);
        int len = 0;
        do {
            len = cbuffer_get_msg(&buff, msg, sizeof(msg));
            if(len > 0){
                printf("%s connected\n", &msg[sizeof(int) + 3]);
                user_count -= 1;
            }
        } while(len > 0);
    }
    printf("You are connected!!!\nType @EXIT to quit, @m <sender> <receiver> <msg> to send msg to another user, #f <sender> <receiver> <filename> to send file\n");
    pthread_t tid; 
    pthread_create(&tid, NULL, &run, (void *) &connSock);
    char cmd[100] = {0};
    char sender[100] = {0};
    char recv[100] = {0};
    char ctx[BUFFER_SIZE] = {0};

    while(1){//waiting for user input
        scanf("%[^\n]%*c", sentBuf);
        if (strcmp(sentBuf,"@EXIT") == 0){
            sprintf(msg, "#q %s #", name);
            int len = create_msg(sentBuf, msg);
            send(connSock, sentBuf, len, 0);
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
                    sprintf(msg, "%s %s %s %s %d", cmd, sender, recv, ctx, size);
                    int len = create_msg(sentBuf, msg);
                    send(connSock, sentBuf, len, 0);
                    int bytes_read = 0;
                    FILE *f = fopen(ctx, "rb");
                    char data[100] = {0};
                    cmd[1] = 'd';
                    while(!feof(f)){
                        sprintf(msg, "%s %s %s ", cmd, sender, recv);
                        int _idx = strlen(msg);
                        bytes_read = fread(data, 1, BUFFER_SIZE - _idx - sizeof(int), f);
                        count +=  bytes_read;
                        if(bytes_read < 0){
                            break;
                        }
                        memmove(&msg[_idx], data, bytes_read);
                        int len = bytes_read + _idx + sizeof(int);
                        memmove(sentBuf, &len, sizeof(int));
                        memmove(&sentBuf[sizeof(int)], msg, len-sizeof(int) );
                        send(connSock, sentBuf, len, 0);
                    }
                    fclose(f);
                }
                printf("file sent!!!\n");

            }
            else if(cmd[1] == 'm'){
                int msg_len = strlen(sentBuf) + sizeof(int);
                printf("msg len %d\n", msg_len);
                memmove(msg, &msg_len, sizeof(int));
                memmove(&msg[sizeof(int)], sentBuf, strlen(sentBuf));
                send(connSock, msg, msg_len, 0); 
            }
        }
    }

    return 1;
}
