#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFFERSIZE 1024
typedef unsigned char BYTE;
pthread_t receiveID;

void receive(void *argv)
{
    int sockclient = *(int*)(argv);
    BYTE recvbuff[BUFFERSIZE];
    while(recv(sockclient, recvbuff, sizeof(recvbuff), 0)!=-1) //receive
    {
        fputs(recvbuff, stdout);
        fputs("\n", stdout);      
    }
    fprintf(stderr, "Receive eroor: %s(errno: %d)\n", strerror(errno), errno);
}
int main()
{
    ///define sockfd
    int sockclient = socket(AF_INET,SOCK_STREAM, 0);
    ///definet sockaddr_in
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    
    int isConn = 0;
    
    BYTE buff[BUFFERSIZE];
    
    while (fgets(buff, sizeof(buff), stdin) != NULL)
    {
        char cmd[100], ip[100];
        int port;
        if(sscanf(buff, "%s", cmd) == -1)    //command error
        { 
            fprintf(stderr, "Input eroor: %s(errno: %d) And please input again\n", strerror(errno), errno);
            continue;
        }
        if(strcmp(cmd, "conn") == 0)        //connecton command
        {
            char ip[100];
            int port, ipLen=0;
            if(sscanf(buff+strlen(cmd)+1, "%s", ip) == -1)    //command error
            { 
                fprintf(stderr, "Input eroor: %s(errno: %d) And please input again\n", strerror(errno), errno);
                continue;
            }
            if((sscanf(buff+strlen(cmd)+strlen(ip)+2, "%d", &port)) == -1)    //command error
            { 
                fprintf(stderr, "Input eroor: %s(errno: %d) And please input again\n", strerror(errno), errno);
                continue;
            }  
           // fprintf(stdout, "%s %d\n",ip, port);          
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons(port);  ///server port
            servaddr.sin_addr.s_addr = inet_addr(ip);  //server ip
            if (connect(sockclient, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
            {
                fprintf(stderr, "Connect eroor: %s(errno: %d)\n", strerror(errno), errno);
                continue;
            } 
            fprintf(stdout, "Connect successfully\n"); 
            isConn = 1;  
            pthread_create(&receiveID, NULL, (void *)(receive), (void *)(&sockclient));            
        }
        else if(strcmp(cmd, "disconn") == 0)
        {
            if(isConn == 0)
            {
                fprintf(stdout, "There is not a connection!\n");
                continue;
            }
            else
            {
                pthread_cancel(receiveID);
                close(sockclient);
            }   
            isConn = 0;
        }
        else if(strcmp(cmd, "quit") == 0)
        {
            if(isConn)
            {
                pthread_cancel(receiveID);
                close(sockclient);
            }
            return 0;
        }
        else
        {
            if(send(sockclient, buff, strlen(buff)+1, 0) == -1) //send
            {
                fprintf(stderr, "Send eroor: %s(errno: %d)\n", strerror(errno), errno);
                continue;
            }
            
            if(isConn == 0)
            {
                fprintf(stdout, "Please use conn <ip> <port> command to build a connnection!\n");
                continue;
            }
            memset(buff, 0, sizeof(buff));
        }      
    }
    close(sockclient);
    return 0;
}