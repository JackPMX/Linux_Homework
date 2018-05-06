#include<sys/types.h>
#include<sys/socket.h>
#include<pthread.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<linux/in.h>
#include<errno.h>
#include<stdio.h>
#include<fcntl.h>



#define MAXCONN 10
#define BACKLOG 5
#define PORT 1799
#define BUFFSIZE 1024

typedef struct ClientInfo{
    struct sockaddr_in addr;
    int isconn;
    int index;
    int clientfd;
}ClientInfo;

pthread_mutex_t activeConnMutex;
pthread_mutex_t clientsMutex[MAXCONN];
pthread_cond_t connDis;
pthread_t threadID[MAXCONN];
pthread_t serverManagerID[MAXCONN];
ClientInfo clients[MAXCONN];

int serverExit=0;

void tolowerString(char *s)
{
    int i=0;
    while(i < strlen(s))
    {
        s[i] = tolower(s[i]);
        ++i;
    } 
}

void clientManager(void* argv){
    ClientInfo *client=(ClientInfo *)(argv);
    unsigned char buff[BUFFSIZE];
    int recvbytes;
    int i=0;
    int clientfd=client->clientfd;
    struct sockaddr_in addr = client->addr;
    int isconn=client->isconn;
    int index=client->index;
    printf("tag1\n");
    while((recvbytes = recv(clientfd, buff, BUFFSIZE, 0)) != -1){
        tolowerString(buff);
        printf("Shuchu buff1 %s\n",buff);
        char cmd[100];
        if((sscanf(buff,"%s",cmd))==-1){
            char err[100];
            strcpy(err, "Error command and please enter again!\n");
            if(send(clientfd,err,strlen(err)+1,0)==-1){
                fprintf(stdout,"Send errinfo failed!\n");
            }
        }
        else{
            char msg[BUFFSIZE];
            int dest=index;
            int ismsg=0;
            if((strcmp(cmd,"disconn")==0)){
                pthread_cond_signal(&connDis);
                break;
            }
            else if(strcmp(cmd,"send")==0){
                if(sscanf(buff,"%d%s",&dest,msg)==-1||dest>=MAXCONN){
                    fprintf(stderr,"send meg failed!\n");
                    break;
                }
                fprintf(stdout,"%d  %s",dest,msg);
                ismsg=1;
            }
            else{
                char err[100];
                strcpy(err, "Unknown command and please enter again!\n");
                fprintf(stderr, "Send to %d message eroor: %s(errno: %d)\n", clientfd, strerror(errno), errno);
                break;
            }

            if(ismsg==1){
                pthread_mutex_lock(&clientsMutex[dest]);
                if(clients[dest].isconn==1){
                    if(send(clients[dest].clientfd,msg,strlen(msg)+1,0)==-1){
                        fprintf(stderr,"Send msg to %d failed, errinfo : %s, error no :%d\n",dest,strerror(errno),errno);
                        pthread_mutex_unlock(&clients[dest].clientfd);
                        break;
                    }
                    else{
                        printf("Send successfully!\n");
                        pthread_mutex_unlock(&clientsMutex[dest]); 
                    }
                }
                else{
                    sprintf(msg, "The destination is disconneted!");
                    dest = index; 
                }
            }
        }
    }
    printf("tag2\n");
    pthread_mutex_lock(&clientsMutex[index]);
    client->isconn=0;
    pthread_mutex_unlock(&clientsMutex[index]);
    printf("tag3\n");
    if(close(clientfd)==-1){
        fprintf(stderr, "Close %d client eroor: %s(errno: %d)\n", clientfd, strerror(errno), errno);
    }
    fprintf(stderr, "Client %d connetion is closed\n", clientfd);
    printf("tag4\n");
    pthread_exit(NULL);
}

int main(int argc, char** argv){
    //initialize the mutex 
    int activeconn=0;
    pthread_mutex_init(&activeConnMutex,NULL);
    pthread_cond_init(&connDis,NULL);
    int i=0;
    for(i=0;i<MAXCONN;i++){
        pthread_mutex_init(&clientsMutex[i],NULL);
    }
    for(i=0;i<MAXCONN;i++){
        clients[i].isconn=0;
    }

    int listenfd=0;
    struct sockaddr_in servaddr;

    
    if((listenfd=socket(AF_INET,SOCK_STREAM,0))==-1){
        fprintf(stderr,"Create listen_socket error: %s,errno: %d\n",strerror(errno),errno);
        exit(0);
    }
    else{
        fprintf(stdout,"Create listen_socket successfully\n");    
        fcntl(listenfd,F_SETFL,O_NONBLOCK);   
    }
 
    //bind 
    memset(&servaddr, 0, sizeof(servaddr));  //initialize the server address 
    servaddr.sin_family = AF_INET;           //AF_INET means using TCP protocol
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);    //any in address(there may more than one network card in the server)
    servaddr.sin_port = htons(PORT);            //set the port
   
    if(bind(listenfd, (struct sockaddr*)(&servaddr), sizeof(servaddr)) == -1)
    {
        fprintf(stderr, "Bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
    }
    else
        fprintf(stdout, "Bind socket successfully\n");

    //listening
    if(listen(listenfd,BACKLOG)==-1){
        fprintf(stderr,"Listen failed: %s,errno: %d\n",strerror(errno),errno);
        exit(0);
    }
    else{
        fprintf(stdout, "Listen successfully\n");
    }


    //Waiting for conn
    while(1){
        pthread_mutex_lock(&activeConnMutex);
        if(activeconn >= MAXCONN){
            pthread_cond_wait(&connDis,&activeConnMutex);
        }
        pthread_mutex_unlock(&activeConnMutex);

        //find an empty postion for a new connnetion
        int i=0;
        for(i=0;i<MAXCONN;i++){
            pthread_mutex_lock(&clientsMutex[i]);
            if(clients[i].isconn==0){
                pthread_mutex_unlock(&clientsMutex[i]);
                break;
            }
            pthread_mutex_unlock(&clientsMutex[i]);
        }

        struct sockaddr_in addr;
        int clientfd=0;
        int sinsize=sizeof(struct sockaddr_in);
        if(clientfd=accept(listenfd,(struct sockaddr*)&addr,&sinsize)==-1){
            sleep(1);
            continue;
        }
        else{
            fprintf(stdout,"Accept successfully!Client is %d\n",i);
        }

        pthread_mutex_lock(&clientsMutex[i]);
        clients[i].index=i;
        clients[i].clientfd=clientfd;
        clients[i].isconn=1;
        clients[i].addr=addr;
        pthread_mutex_unlock(&clientsMutex[i]);

        //create a thread for a client
        pthread_create(&threadID[i],NULL,(void *)clientManager,&clients[i]);
    }

}