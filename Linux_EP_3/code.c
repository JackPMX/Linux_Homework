#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <utime.h>

#define isFile 8
#define isFolder 4

char parsecmd(char *);
void copyfolder(char *isrc ,char *itar);
void copyfile(char *isrc ,char *itar);

int main(int argc, char ** argv)
{
  char cmd[128];
  char tmp[101];
  char b = 'F';
  char * p;
  int count=0;
  char his[32][128];
  memset(his, 0, sizeof(his));

  while(1) {
    memset(cmd, 0, 1024);
    printf("Next command%% ");
    fgets(cmd, 1000, stdin);
    cmd[strlen(cmd) - 1] = 0;

    b = parsecmd(cmd);

    if(b=='T') {
        printf("I will quit\n");
        break;
    }

    switch(b){
        case 1:{
            memset(his[count],0,128);
            strcpy(his[count],cmd);
            count=(count+1)%32;
            if(chdir(cmd + 3) != 0) {
                printf("chdir(%s) error!%s\n", cmd + 3, strerror(errno));
            }
            printf("I'm working in '%s' now\n", getcwd(tmp, 100));
            break;
        }
        case 3:{
            memset(his[count],0,128);
            strcpy(his[count],cmd);
            count=(count+1)%32;
            printf("%d\n", getpid());
            break;
        }
        case 4:{
            memset(his[count],0,128);
            strcpy(his[count],cmd);
            count=(count+1)%32;
            remove(cmd + 3);
            break;
        }
        case 5:{
            memset(his[count],0,128);
            strcpy(his[count],cmd);
            count=(count+1)%32;
            mkdir(cmd + 6, 0755);
            break;
        }
        case 6:{
            memset(his[count],0,128);
            strcpy(his[count],cmd);
            count=(count+1)%32;
             p = strchr(cmd + 3, ' ');
            *p = 0;
            rename(cmd + 3, p + 1);
            break;
        }
        case 2:{
            DIR* dirinfo;
            dirinfo=opendir(".");
            struct dirent *entry;
            while((entry=readdir(dirinfo))!=NULL){
                if(strcmp(entry->d_name,"..")==0||strcmp(entry->d_name,".")==0){
                    continue;
                }
                else{
                    /*
                    if (strcmp(entry->d_name,".")==0) {
                      printf("fuck the .\n");
                    }
                    */
                    if(entry->d_type==isFile){
                        printf("FILE   %s\n",entry->d_name);
                    }
                    if(entry->d_type==isFolder){
                        printf("FOLDER %s\n",entry->d_name);
                    }
                }
            }
            memset(his[count],0,128);
            strcpy(his[count],cmd);
            count=(count+1)%32;
            break;
        }
        case 7:{
            char *p;
            char *dest;
            char *src;
            p = strtok (cmd," "); 
            p = strtok (NULL," "); 
            printf ("%s\n",p); 
            strcpy(src,p);
            p = strtok(NULL,p);    
            printf ("%s\n",p); 
            strcpy(dest,p);      
            getchar(); 
            copyfolder(src,dest);
            memset(his[count],0,128);
            strcpy(his[count],cmd);
            count=(count+1)%32;
            break;
        }
        case 8:{
            char * dir;
            dir=get_current_dir_name();
            printf("%s\n",dir);
            memset(his[count],0,128);
            strcpy(his[count],cmd);
            count=(count+1)%32;
            break;
        }
        case 9:{
            int i=0;
            for (i=0;i<32;i++){
                if(strcmp(his[i],"")==0){
                    continue;
                }
                else{
                    printf("%s\n",his[i]);
                }
            }
            break;
        }
        case 0:{
            printf("Bad command, try again!\n");
            break;
        }
    }
  }
  return 0;
}

char parsecmd(char * s)
{
  if(!strcasecmp(s, "exit")) return 'T';
  else if(!strncasecmp(s, "cd", 2)) return 1;
  else if(!strncasecmp(s, "ls", 2)) return 2;
  else if(!strncasecmp(s, "pwd", 3)) return 8;
  else if(!strncasecmp(s, "pid", 3)) return 3;
  else if(!strncasecmp(s, "rm", 2)) return 4;
  else if(!strncasecmp(s, "mkdir", 5)) return 5;
  else if(!strncasecmp(s, "mv", 2)) return 6;
  else if(!strncasecmp(s, "cp", 2)) return 7;
  else if(!strcmp(s, "history" )) return 9;
  else return 0;
}

void copyfolder(char *isrc, char *itar){
    printf("enter copyfolder\n");
    printf("isrc is %s      itar is %s\n", isrc,itar);
    struct stat statinfo;
    struct utimbuf timeinfo;
    DIR * dirinfo;
    stat(isrc,&statinfo);
    /*
    stat(const char *file_name,struct stat *buf)
    统计文件名指定的文件属性信息
    */
    mkdir(itar,statinfo.st_mode);
    /*
    mkdir(const char * dir_pathname,mode_t mode)
    */
    timeinfo.actime=statinfo.st_atime;
    timeinfo.modtime=statinfo.st_mtime;
    utime(itar,&timeinfo);
    /*utime函数:修改文件的存取和修改时间
    int utime(const char *filename,const struct utimbuf buf);
    */
  
    struct dirent *entry;
    /*
    struct dirent {
      long d_ino;                 Always zero
      unsigned short d_reclen;    Structure size
      size_t d_namlen;            Length of name without \0
      int d_type;                 File type
      char d_name[PATH_MAX];      File name
      };
    */
    dirinfo=opendir(isrc);
    while((entry=readdir(dirinfo))!=NULL){
    if(strcmp(entry->d_name,"..")==0||strcmp(entry->d_name,".")==0){
      continue;
    }else{
      /*
      if (strcmp(entry->d_name,".")==0) {
        printf("fuck the .\n");
      }
      */
      if(entry->d_type==isFile){
        char src[512];
        char tar[512];
        strcpy(src,isrc);
        strcat(src,"/");
        strcat(src,entry->d_name);
        strcpy(tar,itar);
        strcat(tar,"/");
        strcat(tar,entry->d_name);
        copyfile(src,tar);
      }
      if(entry->d_type==isFolder){
        char src[512];
        char tar[512];
        strcpy(src,isrc);
        strcat(src,"/");
        strcat(src,entry->d_name);
        strcpy(tar,itar);
        strcat(tar,"/");
        strcat(tar,entry->d_name);
        copyfolder(src,tar);
      }
    }
    }
  }

void copyfile(char *isrc, char *itar){
    printf("enter copyfile\n" );
    printf("isrc is %s      itar is %s\n", isrc,itar);
    /*
    int open(const char *patename,int_oflg[,mode_t mode])
    若正确，返回文件描述符
    */
    int fdsrc=open(isrc,0);
    int fdtar=0;
    char buffer[1024];
    struct stat statinfo;
    struct utimbuf timeinfo;
  
    stat(isrc,&statinfo);
  
    /*creat
    int creat(const char *pathname,mode_t mode);
    若正确，返回文件描述符
    */
    fdtar=creat(itar,statinfo.st_mode);
  
    int writebuf=0;
    /*read
    ssize_t read(int fd,void *buf,size_t nbytes);
    正确为0或读写的字节数
    */
  
    while((writebuf=read(fdsrc,buffer,1024))>0){
      /*write
      ssize_t write(int fd,void *buf,size_t nbytes);
      */
      write(fdtar,buffer,writebuf);
    }
    close(fdtar);
    close(fdsrc);
  }