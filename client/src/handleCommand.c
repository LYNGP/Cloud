#include"handleCommand.h"

int verdictCommand(int sockfd,const char* command,const char* ip,const char* port,const char* username){
     //切分命令和参数
      char tempCommand[4096] = {0};
      memcpy(tempCommand,command,strlen(command));
      char* order;
      char* arguments[1024];
      int arglen;
      arglen = 0;
      char* saveptr;
      char argv[4096] = {0};
      order = strtok_r(tempCommand," ",&saveptr);
      memcpy(argv,saveptr,strlen(saveptr));
      if(order!=NULL){
          ++arglen;
      }
      //printf("order = %s,strlen(order)= %ld,argc = %d\n",order,strlen(order),arglen);
      //printf("saveptr = %s\n",saveptr);
      for(int i=0;i<1024;++i){
          arguments[i] = strtok_r(NULL," ",&saveptr);
          if(arguments[i]==NULL){
              break;
          }
          //printf("%s\n",arguments[i]);
          ++arglen;
      }
      //printf("arglen = %d\n",arglen);
// 判断是哪一种命令并且执行相应的函数
    if(memcmp("cd",order,strlen(order))==0){
        //printf("cd will execute!\n");
        if (arglen != 2){
            return -1;
        }
        sendCommand(sockfd,command);
        clientExecCd(sockfd,argv);
        return 0;
    }
    if(memcmp("ls",order,strlen(order))==0){
        //printf("ls will execute!\n");
        if (arglen != 2){
            return -1;
        }
        sendCommand(sockfd,command);
        clientExecLs(sockfd,argv);
        return 0;
    }
    if(memcmp("pwd",order,strlen(order))==0){
        //printf("pwd will execute!\n");
        if (arglen != 1){
            return -1;
        }
        sendCommand(sockfd,command);
        char pwd[4096] = {0};
        //char curStackPath[4096] = {0};
        //char username[1024] = "zhangSan";
        clientExecPwd(sockfd,pwd);
        //sprintf(pwd,"%s/%s",username,curStackPath);
        printf("%s\n",pwd);
        return 0;
    }
    if(memcmp("puts",order,strlen(order))==0){
        //printf("puts will execute!\n");
        if (arglen != 2){
            return -1;
        }
        sendCommand(sockfd,command);
        clientExecPuts(sockfd,argv);
        return 0;
    }
    if(memcmp("gets",order,strlen(order))==0){
        //printf("gets will execute!\n");
        if (arglen != 2){
            return -1;
        }
        sendCommand(sockfd,command);
        clientExecGets(sockfd,argv);
        return 0;
    }
    if(memcmp("rm",order,strlen(order))==0){
        //printf("rm will execute!\n");
        if (arglen != 2){
            return -1;
        }
        sendCommand(sockfd,command);
        clientExecRm(sockfd,argv);
        return 0;
    }
    if(memcmp("mkdir",order,strlen(order))==0){                                                            
        //printf("mkdir will execute!\n");
        if (arglen != 2){
            return -1;
        }
        sendCommand(sockfd,command);
        clientExecMkdir(sockfd,argv);
        return 0;
    }
    if(memcmp("rmdir",order,strlen(order))==0){
        //printf("is rmdir!\n");
        if (arglen != 2){
            return -1;
        }
        sendCommand(sockfd,command);
        clientExecRmdir(sockfd,argv);
        return 0;
    }
    if(memcmp("reminder",order,strlen(order))==0){
        if(arglen != 1){
            return -1;
        }
        sendCommand(sockfd,command);
        getReminder(sockfd,username,ip,port);
        return 0;
    }
    return 1;
}

int clientExecCommand(int sockfd,const char* command,const char* ip,const char* port,const char* username){
   
    int ret = verdictCommand(sockfd,command,ip,port,username);
    //printf("ret = %d\n",ret);
    if(ret == 1){
        printf("Failed,your command is error!\n");
        return 1;
    }
    if(ret == -1){
        printf("Failed,your numbers of argument is error!\n");
        return 1;
    }
    //printf("before sockfd = %d,command = %s\n",sockfd,command);
    return 0;
}

int sendCommand(int sockfd,const char* command){
    train_t train;
    memset(&train,0,sizeof(train_t));
    train.length = strlen(command)+1;
    //printf("train.legth = %d\n",train.length);
    send(sockfd,&train.length,sizeof(train.length),0);
    memcpy(train.data,command,train.length);
    send(sockfd,&train.data,train.length,0);
    return 0;
}

int clientExecCd(int sockfd,const char* dirName){
    train_t train;
    memset(&train,0,sizeof(train_t));
    recvn(sockfd,&train.length,sizeof(train.length));
    recvn(sockfd,train.data,train.length);
    //服务器上不存在目标文件夹
    //printf("train.data = %s\n",train.data);
    if(memcmp(train.data,"-1",strlen("-1"))==0){
        printf("cd: %s : No such directory\n",dirName);
        return -1;
    }
    if(memcmp(train.data,"-2",sizeof("-2"))==0){
        printf("Failed:Database operation failure\n");
        return -1;
    }
    //服务器上有对应的文件夹
    //printf("train.data = %s\n",train.data);
    if(memcmp(train.data,"0",strlen("0"))==0){
        printf("success!\n");
        return 0;
    }
    return 0;
}

int clientExecLs(int sockfd,const char* dirName){
    //printf("I am clientExecLs,dirName = %s\n",dirName);
    train_t train;
    while(1){
        //接收文件的信息
        memset(&train,0,sizeof(train_t));

        recvn(sockfd,&train.length,sizeof(train.length));
        recvn(sockfd,&train.data,train.length);
        if(memcmp(train.data,"0",train.length)==0){
            break;
        }
        if(memcmp(train.data,"-1",train.length)==0){
            printf("ls: cannot access '%s': No such directory\n",dirName);
            break;
        }
        printf("%s\n",train.data);
    }
    return 0;
}

int clientExecPwd(int sockfd,char * pwd){
    //printf("I am clientExecPwd\n");
    
    // 接收当前路径
    train_t train;
    memset(&train,0,sizeof(train_t));
    recvn(sockfd,&train.length,sizeof(train.length));
    recvn(sockfd,train.data,train.length);
    memcpy(pwd,train.data,train.length);
    return 0;
}


int clientExecRm(int sockfd,const char* filename){
    //printf("I am clientExecRm,filename = %s\n",filename);
    // 接收删除的状态
    train_t train;
    memset(&train,0,sizeof(train_t));
    recvn(sockfd,&train.length,sizeof(train.length));
    recvn(sockfd,train.data,train.length);
    if(memcmp(train.data,"-1",sizeof("-1"))==0){
        printf("rm: cannot remove '%s': No such file\n",filename);
        return -1;
    }
    if(memcmp(train.data,"-2",sizeof("-2"))==0){
        printf("Failed:Database operation failure\n");
        return -1;
    }
    if(memcmp(train.data,"0",sizeof("0"))==0){
        printf("Success\n");
        return 0;
    }
    return 0;
}

int clientExecMkdir(int sockfd,const char* dirpath){
    //printf("I am clientExecMkdir,dirpath = %s\n",dirpath);
    train_t train;
    memset(&train,0,sizeof(train_t));
    recvn(sockfd,&train.length,sizeof(train.length));
    recvn(sockfd,train.data,train.length);
    if(memcmp(train.data,"-1",sizeof("-1"))==0){
        printf("Failed:mkdir %s: please check your path\n",dirpath);
        return -1;
    }
    if(memcmp(train.data,"-2",sizeof("-2"))==0){
        printf("Failed:Database operation failure\n");
        return -1;
    }
    if(memcmp(train.data,"-3",sizeof("-3"))==0){
        printf("Failed:The directory already exists\n");
        return -1;
    }
    if(memcmp(train.data,"0",sizeof("0"))==0){
        printf("Success\n");
        return 0;
    }
    return 0;
}

int clientExecRmdir(int sockfd,const char* dirpath){
    //printf("I am clientExecRmdir,dirpath = %s\n",dirpath);
    train_t train;
    memset(&train,0,sizeof(train_t));
    recvn(sockfd,&train.length,sizeof(train.length));
    recvn(sockfd,train.data,train.length);
    if(memcmp(train.data,"1",sizeof("1"))==0){
        printf("rm: refusing to remove '.' or '..'\n");
        return -1;
    }
    if(memcmp(train.data,"-1",sizeof("-1"))==0){
        printf("Failed: please check your path\n");
        return -1;
    }
    if(memcmp(train.data,"0",sizeof("0"))==0){
        printf("Success\n");
        return 0;
    }

    return 0;
}
int clientExecPuts(int sockfd,const char* filename){
    //printf("i am putBigFile\n");
    train_t train;
    // 打开文件
    int fd = open(filename,O_RDWR);
    if(fd == -1){
        //无法打开文件，通知服务端错误
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-1");
        memcpy(train.data,"-1",train.length);
        send(sockfd,&train.length,sizeof(train.length),0);
        send(sockfd,train.data,train.length,0);
        printf("File opening failure\n");
        return -1;
    }
    
    //计算sha1sum
    char sha1sum[256] = {0};
    generateSha1sum(filename,sha1sum,41);
    printf("sha1sum = %s\n",sha1sum);

    //将sha1sum发送给服务端
    memset(&train,0,sizeof(train_t));
    train.length = strlen(sha1sum);
    memcpy(train.data,sha1sum,train.length);
    send(sockfd,&train.length,sizeof(train.length),0);
    send(sockfd,train.data,train.length,0);
    
    //接收服务端是否有一模一样的文件
    memset(&train,0,sizeof(train_t));
    recvn(sockfd,&train.length,sizeof(train.length));
    recvn(sockfd,train.data,train.length);
    if(memcmp(train.data,"0",train.length)==0){
        //服务端有一模一样的文件,秒传
        //接收秒传是否成功
        memset(&train,0,sizeof(train_t));
        recvn(sockfd,&train.length,sizeof(train.length));
        recvn(sockfd,train.data,train.length);
        if(memcmp(train.data,"0",train.length)==0){
            printf("Success!\n");
            return 0;
        }else{
            printf("Failed!\n");
                return -1;
        }
    }
    if(memcpy(train.data,"-1",train.length)==0){
        printf("Failed!\n");
        return -1;
    }

    //服务端没有一模一样的文件
    //------------------------------mmap传输模式---------------------------------------
    //发送文件大小
    struct stat statbuf;
    fstat(fd,&statbuf);
    off_t filesize = statbuf.st_size;
    off_t allocateSize = ((filesize/4096)+1)*4096;
    memset(&train,0,sizeof(train_t));
    train.length = sizeof(statbuf.st_size);
    memcpy(&train.data,&statbuf.st_size,sizeof(statbuf.st_size));
    send(sockfd,&train.length,sizeof(train.length),0);
    send(sockfd,train.data,train.length,0);
    printf("filesize = %ld\n",filesize);
    char* pmmap = (char *)mmap(NULL,allocateSize,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    //发送文件
    send(sockfd,pmmap,filesize,0);
    munmap(pmmap,allocateSize);
    //查看接受进度
    while(1){
        memset(&train,0,sizeof(train_t));
        recvn(sockfd,&train.length,sizeof(train.length));
        recvn(sockfd,&train.data,train.length);
    if(memcmp(train.data,"-1",train.length)== 0){
        printf("upload Failed!\n");
        return -1;
    }
    if(memcmp(train.data,"0",train.length)== 0){
        printf("100.00%%\n");
        return 0;
    }
        printf("%s\r",train.data);
        fflush(stdout);
    }
    //接收是否上传成功
        memset(&train,0,sizeof(train_t));
        recvn(sockfd,&train.length,sizeof(train.length));
        recvn(sockfd,train.data,train.length);
        if(memcmp(train.data,"0",train.length)==0){
            return 0;
        }else{
            printf("Failed!\n");
                return -1;
        }
    return 0;
}
int clientExecGets(int sockfd,const char* filename){
    //printf("i am getsBigFile\n");
    //切换到对应的目录
    chdir("netDisk");

    //---------------------------------------------接收大文件------------------------------------------
    train_t train;
    //服务器上有文件是否存在 
    memset(&train,0,sizeof(train_t));
    recvn(sockfd,&train.length,sizeof(train.length));
    recvn(sockfd,train.data,train.length);
    //printf("train.data = %s\n",train.data);
    if(memcmp(train.data,"-2",train.length)==0){
        printf("Database operation failure!\n");
        chdir("../");
        return -1;
    }
    if(memcmp(train.data,"-1",train.length)==0){
        printf("no this file\n");
        chdir("../");
        return -1;
    }
    //文件存在接收文件的长度
    off_t filesize;
    //memset(&train,0,sizeof(train_t));
    //recvn(sockfd,&train.length,sizeof(train.length));
    //recvn(sockfd,train.data,train.length);
    memcpy(&filesize,train.data,train.length);
    //printf("filesize = %ld\n",filesize);
    

    //判断文件是否存在
    int ret = access(filename,F_OK);
    off_t lastTotal = 0;
    int fd;
    if(ret == 0){
        fd = open(filename,O_RDWR|O_APPEND);
        //获取文件长度
        lastTotal = lseek(fd,0,SEEK_END);
    }else{
        fd = open(filename,O_RDWR|O_CREAT,0666);
    }
    //将文件的长度发送给服务端
    memset(&train,0,sizeof(train_t));
    train.length = sizeof(lastTotal);
    memcpy(train.data,&lastTotal,train.length);
    send(sockfd,&train.length,sizeof(train.length),0);
    send(sockfd,train.data,train.length,0);

    off_t total = lastTotal;
    while(1){
        char buf[40960] = {0};
        //printf("lastTotal = %ld\n",lastTotal);
        if(filesize - lastTotal >= 40960){
            int ret = recvn(sockfd,buf,40960);
            if(ret == 1){
                //接收失败，通知客户端
                memset(&train,0,sizeof(train_t));
                train.length = strlen("-1");
                memcpy(train.data,"-1",train.length);
                send(sockfd,&train.length,sizeof(train.length),0);
                send(sockfd,&train.data,train.length,0);
                return -1;
            }
            write(fd,buf,40960);
            total += 40960;
        }
        if(filesize - lastTotal == 0){
            break;
        }
        if(filesize - lastTotal < 40960){
            recvn(sockfd,buf,filesize - lastTotal);
            write(fd,buf,filesize - lastTotal);
            total += filesize - lastTotal;
        }
        lastTotal = total;
        //打印下载进度
        printf("%6.2lf%%\r",lastTotal*100.0/filesize);
        fflush(stdout);
    }
    close(fd);
    //下载完成通知服务端
    memset(&train,0,sizeof(train_t));
    train.length =strlen("0");
    memcpy(train.data,"0",strlen("0"));
    send(sockfd,&train.length,sizeof(train.length),0);
    send(sockfd,train.data,train.length,0);

    //切换回当前目录
    chdir("../");
    if(memcmp(train.data,"0",train.length)==0){
        printf("100.00%%\n");
    }
    return 0;

} 

int getReminder(int sockfd,const char* username,const char* ip,const char* port){
    char reminder[4096] = {0};
    char pwd[4096] = {0};
    clientExecPwd(sockfd,pwd);
    sprintf(reminder,"%s@%s:%s:~%s$ ",username,ip,port,pwd);
    printf("%s",reminder);
    return 0;
}

int generateSha1sum(const char* filename,char* sha1sum,int length){
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    int fd = open(filename,O_RDWR);
    char buf[4096];
    while(1){
        bzero(buf,sizeof(buf));
        ssize_t sret = read(fd,buf,sizeof(buf));
        if(sret == 0){
            break;
        }
        SHA1_Update(&ctx,buf,sret);
    }
    unsigned char md[20] = {0};
    //md将要存储哈希值的二进制形式
    SHA1_Final(md,&ctx);
    char sha1[40] = {0};
    char tempsha1[40] = {0};
    for(int i=0;i<20;++i){
        //printf("%02x",md[i]);
        bzero(tempsha1,sizeof(tempsha1));
        sprintf(tempsha1,"%s%02x",sha1,md[i]);
        memcpy(sha1,tempsha1,sizeof(sha1));
    }
    //printf("\n");
    memcpy(sha1sum,sha1,length);
    return 0;
}

