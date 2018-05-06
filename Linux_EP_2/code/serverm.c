#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#define PORT  7777
#define BACKLOG 10
#define MAXCONN 10
#define BUFFSIZE 1024

typedef unsigned char BYTE;
typedef struct ClientInfo
{
    struct sockaddr_in addr;
    int clientfd;
    int isConn;
    int index;
} ClientInfo;
pthread_mutex_t activeConnMutex;
pthread_mutex_t clientsMutex[MAXCONN];
pthread_cond_t connDis;
pthread_t threadID[MAXCONN];
pthread_t serverManagerID;
ClientInfo clients[MAXCONN];
int serverExit = 0;
/*@brief Transform the all upper case 
*
*/

void clientManager(void* argv)
{
    ClientInfo *client = (ClientInfo *)(argv);
    
    BYTE buff[BUFFSIZE];
    int recvbytes;
    
    int i=0;
    int clientfd = client->clientfd;
    struct sockaddr_in addr = client->addr;
    int isConn = client->isConn;
    int clientIndex = client->index;
    while((recvbytes = recv(clientfd, buff, BUFFSIZE, 0)) != -1)
    {    
        char cmd[100];
        if((sscanf(buff, "%s", cmd)) == -1)    //command error
        { 
           char err[100];         
           if(send(clientfd, err, strlen(err)+1, 0) == -1)
           {
               strcpy(err, "Error command and please enter again!\n");
               fprintf(stdout, "%d sends an eroor command\n", clientfd);
               break;
           }
        }
        else    
        {
            char msg[BUFFSIZE]; //The message content
            int dest = clientIndex; //message destination
            int isMsg = 0; 
            int isAll = 0;             //any message needed to send
            if(strcmp(cmd, "disconn") == 0)
            {   
                pthread_cond_signal(&connDis);  //send a disconnetion signal and the waiting client can get response  
                break;
            }
            else if(strcmp(cmd, "send") == 0)
            {   
                    
                if(sscanf(buff+strlen(cmd)+1, "%d%s", &dest, msg)==-1 || dest >= MAXCONN)
                {
                    char err[100];
                    strcpy(err, "Destination ID error and please use list to check and enter again!\n");
                    fprintf(stderr, "Close %d client eroor: %s(errno: %d)\n", clientfd, strerror(errno), errno);
                    break;
                }
                printf("%s\n",msg);
                fprintf(stdout, "%d %s\n", dest, msg);   
                isMsg = 1;
                isAll = 0;
            }
            else if(strcmp(cmd,"sendall")==0){
                if(sscanf(buff+8,"%s",msg)==-1){
                    char err[100];
                    strcpy(err, "Destination ID error and please use list to check and enter again!\n");
                    fprintf(stderr, "Close %d client eroor: %s(errno: %d)\n", clientfd, strerror(errno), errno);
                    break;
                }
                fprintf(stdout, "sendall %s\n",  msg);   
                isMsg = 1;
                isAll = 1;
            }
            else
            {
                char err[100];
                strcpy(err, "Unknown command and please enter again!\n");
                fprintf(stderr, "Send to %d message eroor: %s(errno: %d)\n", clientfd, strerror(errno), errno);
                break;
            }
            
            
            if(isMsg)
            {
                if(isAll){
                    int c=0;
                    for(c=0;c<MAXCONN;c++){
                        if(clients[c].isConn==1){
                            pthread_mutex_lock(&clientsMutex[c]);
                            if(send(clients[c].clientfd,msg,strlen(msg)+1,0)==-1){
                                fprintf(stderr, "Send to %d message eroor: %s(errno: %d)\n", clientfd, strerror(errno), errno);
                                pthread_mutex_unlock(&clientsMutex[c]);
                                continue;
                            }
                            pthread_mutex_unlock(&clientsMutex[c]);
                        }
                        else{
                            continue;
                        }
                    }

                } 
                else{
                    pthread_mutex_lock(&clientsMutex[dest]);
                    if(clients[dest].isConn == 0)
                    {
                        sprintf(msg, "The destination is disconneted!");
                        dest = clientIndex; 
                    } 
                            
                    if(send(clients[dest].clientfd, msg, strlen(msg)+1, 0) == -1)
                    {
                        fprintf(stderr, "Send to %d message eroor: %s(errno: %d)\n", clientfd, strerror(errno), errno);
                        pthread_mutex_unlock(&clientsMutex[dest]); 
                        break;
                    }
                    printf("send successfully!\n");
                    pthread_mutex_unlock(&clientsMutex[dest]);
                }
            }
        }  //end else       
    }   //end while
    printf("tag2\n");
    pthread_mutex_lock(&clientsMutex[clientIndex]);
    client->isConn = 0;
    pthread_mutex_unlock(&clientsMutex[clientIndex]);
    printf("tag3\n");
    if(close(clientfd) == -1)
        fprintf(stderr, "Close %d client eroor: %s(errno: %d)\n", clientfd, strerror(errno), errno);
    fprintf(stderr, "Client %d connetion is closed\n", clientfd);
    printf("tag4\n");
    pthread_exit(NULL);
}

int main()
{
   int activeConn = 0;
   
   //initialize the mutex 
   pthread_mutex_init(&activeConnMutex, NULL);   
   pthread_cond_init(&connDis, NULL);
   int i=0;
   for(;i<MAXCONN;++i)
       pthread_mutex_init(&clientsMutex[i], NULL); 
   
   for(i=0;i<MAXCONN;++i)
       clients[i].isConn = 0; 
       

   
   
   int listenfd;
   struct sockaddr_in  servaddr;
    
   //create a socket
   if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
       fprintf(stderr, "Create socket error: %s(errno: %d)\n", strerror(errno), errno);
       exit(0);
   }
   else
       fprintf(stdout, "Create a socket successfully\n");
   
   fcntl(listenfd, F_SETFL, O_NONBLOCK);       //set the socket non-block
   
   //set the server address
   memset(&servaddr, 0, sizeof(servaddr));  //initialize the server address 
   servaddr.sin_family = AF_INET;           //AF_INET means using TCP protocol
   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);    //any in address(there may more than one network card in the server)
   servaddr.sin_port = htons(PORT);            //set the port
   
   //bind the server address with the socket
   if(bind(listenfd, (struct sockaddr*)(&servaddr), sizeof(servaddr)) == -1)
   {
        fprintf(stderr, "Bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        exit(0);
   }
   else
       fprintf(stdout, "Bind socket successfully\n");
   
   //listen
   if(listen(listenfd, BACKLOG) == -1)
   {
       fprintf(stderr, "Listen socket error: %s(errno: %d)\n", strerror(errno), errno);
       exit(0);
   }
   else
       fprintf(stdout, "Listen socket successfully\n");
    
   
   while(1)
   {
       if(serverExit)
       {
           for(i=0;i<MAXCONN;++i)
           {
               if(clients[i].isConn)
               {
                   if(close(clients[i].clientfd) == -1)         //close the client 
                       fprintf(stderr, "Close %d client eroor: %s(errno: %d)\n", clients[i].clientfd, strerror(errno), errno);
                   if(pthread_cancel(threadID[i]) != 0)         //cancel the corresponding client thread
                        fprintf(stderr, "Cancel %d thread eroor: %s(errno: %d)\n", (int)(threadID[i]), strerror(errno), errno);
               }
           }
           return 0;    //main exit;
       }
       
       pthread_mutex_lock(&activeConnMutex);
       if(activeConn >= MAXCONN)
            pthread_cond_wait(&connDis, &activeConnMutex);
       pthread_mutex_unlock(&activeConnMutex);
           
       //find an empty postion for a new connnetion
       int i=0;
       while(i<MAXCONN)
       {
           pthread_mutex_lock(&clientsMutex[i]);
           if(!clients[i].isConn)
           {
               pthread_mutex_unlock(&clientsMutex[i]);
               break;
           }
           pthread_mutex_unlock(&clientsMutex[i]);
           ++i;           
       }   
       
       //accept
       struct sockaddr_in addr;
       int clientfd;
       int sin_size = sizeof(struct sockaddr_in);
       if((clientfd = accept(listenfd, (struct sockaddr*)(&addr), &sin_size)) == -1)
       {   
           sleep(1);        
           //fprintf(stderr, "Accept socket error: %s(errno: %d)\n", strerror(errno), errno);
           continue;
           //exit(0);
       }
       else
           fprintf(stdout, "Accept socket successfully! The client is %d.\n",i);
       
       pthread_mutex_lock(&clientsMutex[i]);
       clients[i].clientfd = clientfd;
       clients[i].addr = addr;
       clients[i].isConn = 1;
       clients[i].index = i;
       pthread_mutex_unlock(&clientsMutex[i]);
       
       //create a thread for a client
       pthread_create(&threadID[i], NULL, (void *)clientManager, &clients[i]);     
       
   }    //end-while
}