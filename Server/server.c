//
//  Dang Van Do
//  17025077

/  server.c
//  waiting for client at port 9999

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


#define SERVER_PORT 9999
#define BUFFER_SIZE 1024
typedef struct client_s {
    char name[100];
    int socket_id;
    struct client_s *prev;
    struct client_s *next;
} client_t;

client_t* clients; 

void init_client(client_t* client){
    memset(client->name, '\0', 100);
    client->socket_id = 0;
    client->prev = NULL;
    client->next = NULL;
}

//function to remove a client from list of clients
void remove_client(client_t* root, int cliSock){
    printf("remove_client\n");
    while(root != NULL){
        if(root->socket_id == cliSock){
            if(root->prev == NULL && root->next == NULL){
                root->socket_id = 0;
            }
            else if(root->prev == NULL){
                clients = root->next;
                clients->prev = NULL;
                free(root);
            }
            else if(root->next == NULL){
                root->prev->next = NULL;
                free(root);
            }
            return;
        }
        root = root->next;
    }
}

//function to add a client to list of clients
void add_client(client_t* root, char* name, int socket){
    if(root->socket_id <= 0){
        memcpy(root->name, name, strlen(name));
        root->socket_id = socket;
    }
    else {
        while(root->next != NULL){
            root = root->next;
        }
        root->next = (client_t*)malloc(sizeof(client_t));
        init_client(root->next);
        memcpy(root->next->name, name, strlen(name));
        root->next->socket_id = socket;
        root->next->prev = root;
    }
}

//create a thread for each client
void *run(void *arg){
    int cliSock = *((int*)arg);
    char 	recvBuf[BUFFER_SIZE];
    char    sentBuf[BUFFER_SIZE] = {0};
    char    msg[2*BUFFER_SIZE];
    int 	n;
    char    username[100] = {0};
    char cmd[100] = {0};
    char sender[100] = {0};
    char receiver[100] = {0};
    client_t* client;
    while (1){
        n = read(cliSock, recvBuf, BUFFER_SIZE-1);
        recvBuf[n] = '\0';
        if (n == 0 || recvBuf[0] == 'q'){ //@EXIT client disconnects
            close(cliSock);
            remove_client(clients, cliSock);
            client = clients;
            while(client != NULL && client->socket_id > 0){//send noti to the others
                sprintf(msg, "q %s", username);
                send(client->socket_id, msg, strlen(msg), 0);
                client = client->next;
            }
            printf("Close the connection\n");
            break;
        }
        else if(recvBuf[0] == 'c'){//client connects
            memcpy(username, &recvBuf[1], strlen(recvBuf) - 1);
            //send noti to others user
            client = clients;
            while(client != NULL){ //check if user is exists or not
                if(client->socket_id > 0 && strcmp(username, client->name) == 0){
                    int retval = -9999;
                    send(cliSock, &retval, sizeof(int), 0);
                    break;
                }
                client = client->next;
            }

            if(client == NULL){//the user is not exists
                int count = 0;
                client = clients;
                while(client != NULL){
                    if(client->socket_id > 0){
                        sprintf(msg, "c %s", username);
                        send(client->socket_id, msg, strlen(msg), 0);
                        count++;
                    }
                    client = client->next;
                }
                add_client(clients, username, cliSock);
                send(cliSock, &count, sizeof(int), 0);
                client = clients;
                while(count > 0 && client->next != NULL){
                    sprintf(msg, "%s\n", client->name);
                    send(cliSock, msg, strlen(msg), 0);
                    client = client->next;
                }
            }
        }
        else {
            sscanf(recvBuf, "%s%s%s", cmd, sender, receiver);

            client = clients;

            if(strcmp(receiver, "#") == 0){//send msg to all of users
                while(client != NULL){
                    if(strcmp(username, client->name) != 0){
                        send(client->socket_id, recvBuf, strlen(recvBuf), 0);
                    }
                    client = client->next;
                }
            }
            else {//send msg to a user
                while(client != NULL){
                    if(strcmp(client->name, receiver) == 0){
                        send(client->socket_id, recvBuf, strlen(recvBuf), 0);
                    }
                    client = client->next;
                }
            }

            if(cmd[1] == 'f'){
                int count = 0;
                char filename[100] = {0};
                int size = 0;
                sscanf(recvBuf, "%s%s%s%s%d", cmd, sender, receiver, filename, &size);
                printf("file size %d\n", size);
                while(size > 0){
                    n = read(cliSock, recvBuf, BUFFER_SIZE);
                    count += n;
                    printf("%d\n", count);
                    client = clients;
                    if(strcmp(receiver, "#") == 0){//send msg to all of users
                        while(client != NULL){
                            if(strcmp(username, client->name) != 0){
                                send(client->socket_id, recvBuf, n, 0);
                            }
                            client = client->next;
                        }
                    }
                    else {//send msg to a user
                        while(client != NULL){
                            if(strcmp(client->name, receiver) == 0){
                                send(client->socket_id, recvBuf, n, 0);
                            }
                            client = client->next;
                        }
                    }
                    size -= n;
                }
                printf("file sent!!!\n");
            }
        }
    }
    return NULL;
}

int main(int argc, char **argv){
    int     listenPort = SERVER_PORT;
    int 	servSock;
    int*    cliSock;
    struct  sockaddr_in servSockAddr, cliAddr;
    int 	servSockLen, cliAddrLen;
    pthread_t tid;
    
    clients = (client_t*) malloc(sizeof(client_t));
    init_client(clients);
    if (argc >= 2){
        listenPort = atoi(argv[1]);
    }
    servSock = socket(AF_INET, SOCK_STREAM,0);
    bzero(&servSockAddr, sizeof(servSockAddr));
    servSockAddr.sin_family = AF_INET;
    servSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servSockAddr.sin_port = htons(listenPort);
    
    servSockLen = sizeof(servSockAddr);
    if(bind(servSock,(struct sockaddr*)&servSockAddr, servSockLen) < 0) {
        perror("bind"); exit(1);
    }
    if(listen(servSock, 10) < 0) {
        perror("listen");
        exit(1);
    }
    while(1){//server waiting for connection from client
        printf("Waiting for a client ... \n");
        cliSock = malloc(sizeof(int));
        *cliSock = accept(servSock, (struct sockaddr *) &cliAddr,
                         (socklen_t *)&cliAddrLen);
        printf("Received a connection from a client %d\n", *cliSock);
        pthread_create(&tid, NULL, &run, (void *) cliSock);
    }
    return 1;
}
