#include"handleCommand.h"

int cmdShareInit(cmdShare_t* pCmdShare,int netfd,MYSQL* mysql,char* username){

    pCmdShare->netfd = netfd;
    pCmdShare->mysql = mysql;
    memcpy(pCmdShare->username,username,strlen(username)+1);
    pathStackInit(&pCmdShare->pathStack);
    return 0;
}

int recvCommand(int netfd,char* command){
    train_t train;
    memset(&train,0,sizeof(train_t));
    int ret = recvn(netfd,&train.length,sizeof(train.length));
    if(ret == 1){
        return -1;
    }
    //printf("train.length = %d",train.length);
    ret = recvn(netfd,command,train.length);
    if(ret == 1){
        return -1;
    }
    //printf("train.data = %s\n",train.data);
    return 0;
}

int serverExecCommand(cmdShare_t* pCmdShare,const char* command){
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
    if(order!=NULL){
        ++arglen;
    }
    memcpy(argv,saveptr,strlen(saveptr));
    for(int i=0;i<1024;++i){
        arguments[i] = strtok_r(NULL," ",&saveptr);
        if(arguments[i]==NULL){
            break;
        }
        ++arglen;    
    }
    // 判断是哪一种命令并且执行相应的函数
    if(memcmp("cd",order,strlen(order))==0){
        //printf("cd will execute! netfd = %d\n",netfd);
        serverExecCd(pCmdShare,argv);
        return 0;
    }

    if(memcmp("ls",order,strlen(order))==0){
        //printf("ls will execute!\n");
        serverExecLs(pCmdShare,argv);
        return 0;
    }
    if(memcmp("pwd",order,strlen(order))==0){
        //printf("pwd will execute!\n");
        serverExecPwd(pCmdShare);
        return 0;
    }
    if(memcmp("puts",order,strlen(order))==0){
        //printf("puts will execute!\n");
        //printf("arguments[arglen-2]=%s,arglen = %d\n",arguments[arglen-2],arglen);
        serverExecPuts(pCmdShare,arguments[arglen-2]);
        return 0;
    }
    if(memcmp("gets",order,strlen(order))==0){
        //printf("gets will execute!\n");
        serverExecGets(pCmdShare,argv);
        return 0;
    }
    if(memcmp("rm",order,strlen(order))==0){
        //printf("rm will execute!\n");
        serverExecRm(pCmdShare,argv);
        return 0;
    }
    if(memcmp("mkdir",order,strlen(order))==0){
        //printf("mkdir will execute!\n");
        serverExecMkdir(pCmdShare,argv);

        return 0;
    }
    if(memcmp("rmdir",order,strlen(order))==0){
        //printf("rmdir will execute!\n");
        serverExecRmdir(pCmdShare,argv);

        return 0;
    }
    if(memcmp("reminder",order,strlen(order))==0){
        serverExecPwd(pCmdShare);
        return 0;
    }

    return 1;
} 

int serverExecCd(cmdShare_t *pCmdShare,const char*dirName){
    //printf("I am serverExecCd,dirName = %s\n",dirName);
    // 分割文件路径
    char tempDirName[4096] = {0};
    memcpy(tempDirName,dirName,strlen(dirName));
    char* arguments[1024];
    int arglen;
    arglen = 0;
    char* saveptr;
    arguments[0] = strtok_r(tempDirName,"/",&saveptr);
    if(arguments[0]!=NULL){
        ++arglen;
    }
    for(int i=1;i<1024;++i){
        arguments[i] = strtok_r(NULL,"/",&saveptr);
        if(arguments[i]==NULL){
            break;
        }
        ++arglen;    
    }


    //存储下当前栈中的路径
    char stackDirname[1024][256];
    int stackDeep = 0;
    pathStackNode_t * pCur = pCmdShare->pathStack.pFront;
    for(int i=0;i<1024;++i){
        if(pCur == NULL){
            break;
        }
        memcpy(stackDirname[i],pCur->dirName,strlen(pCur->dirName)+1);
        pCur = pCur->pNext;
        stackDeep++;
    }

    //获取目标路径
    int dirExist = 1; //0表示目标文件夹不存在
    char targetDir[4096] = {0};
    for(int i=0;i<1024;i++){
        if(arguments[i]==NULL){
            break;
        }
        if(memcmp(arguments[i],"..",strlen(".."))==0){
            if(pCmdShare->pathStack.pFront == NULL){     //该路径下的文件夹不存在
                dirExist = 0;
                break;
            }
            //出栈
            popPathStack(&pCmdShare->pathStack);
        }
        if(memcmp(arguments[i],".",strlen("."))==0){
            continue;
        }
        //入栈
        pushPathStack(&pCmdShare->pathStack,arguments[i]);
    }
    getCurPath(&pCmdShare->pathStack,targetDir);

    //查看目标文件夹是否存在
    train_t train;
    char sql[4096] = {0};
    memset(sql,0,sizeof(sql));
    if(strcmp(targetDir,"")==0){
        sprintf(sql,"select* from file where username = '%s' and tomb = 0 and path is NULL and type = 'D';",
                pCmdShare->username);
    }else{
        sprintf(sql,"select* from file where username = '%s' and tomb = 0 and path = '%s' and type = 'D';",
                pCmdShare->username,targetDir);
    }
    int qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-2");
        memcpy(train.data,"-2",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    MYSQL_RES* res = mysql_store_result(pCmdShare->mysql);
    if(mysql_num_rows(res)<=0){
        dirExist = 0; 
    }

    // 目标文件夹不存在，通知客户端
    if(dirExist == 0){
        // printf("target Dir not exist!\n");

        //恢复栈的内容
        pathStackInit(&pCmdShare->pathStack);
        for(int i=stackDeep-1;i>=0;--i){
            pushPathStack(&pCmdShare->pathStack,stackDirname[i]);
        }
        // 通知客户端
        memset(&train,0,sizeof(train_t));
        memcpy(train.data,"-1",strlen("-1"));
        train.length = strlen("-1");
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }

    //切换成功，通知客户端
    memset(&train,0,sizeof(train_t));
    memcpy(train.data,"0",strlen("0"));
    train.length = strlen("0");
    send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
    send(pCmdShare->netfd,train.data,train.length,0);
    return 0;
}

int serverExecLs(cmdShare_t *pCmdShare,char* dirName){
    /* printf("dirName = %s\n",dirName); */
    // 分割文件路径
    char tempDirName[4096] = {0};
    memcpy(tempDirName,dirName,strlen(dirName));
    char* arguments[1024];
    int arglen;
    arglen = 0;
    char* saveptr;
    arguments[0] = strtok_r(tempDirName,"/",&saveptr);
    if(arguments[0]!=NULL){
        ++arglen;
    }
    for(int i=1;i<1024;++i){
        arguments[i] = strtok_r(NULL,"/",&saveptr);
        if(arguments[i]==NULL){
            break;
        }
        ++arglen;    
    }


    //存储下当前栈中的路径
    char stackDirname[1024][256];
    int stackDeep = 0;
    pathStackNode_t * pCur = pCmdShare->pathStack.pFront;
    for(int i=0;i<1024;++i){
        if(pCur == NULL){
            break;
        }
        memcpy(stackDirname[i],pCur->dirName,strlen(pCur->dirName)+1);
        pCur = pCur->pNext;
        stackDeep++;
    }

    //获取目标路径
    int dirExist = 1; //0表示目标文件夹不存在
    char targetDir[4096] = {0};
    for(int i=0;i<1024;i++){
        if(arguments[i]==NULL){
            break;
        }
        if(memcmp(arguments[i],"..",strlen(".."))==0){
            if(pCmdShare->pathStack.pFront == NULL){     //该路径下的文件夹不存在
                dirExist = 0;
                break;
            }
            //出栈
            popPathStack(&pCmdShare->pathStack);
        }
        if(memcmp(arguments[i],".",strlen("."))==0){
            continue;
        }
        //入栈
        pushPathStack(&pCmdShare->pathStack,arguments[i]);
    }
    getCurPath(&pCmdShare->pathStack,targetDir);
    printf("targetDir = %s\n",targetDir);

    //恢复栈的内容
    pathStackInit(&pCmdShare->pathStack);
    for(int i=stackDeep-1;i>=0;--i){
        pushPathStack(&pCmdShare->pathStack,stackDirname[i]);
    }

    //查看目标文件夹是否存在
    train_t train;
    char sql[4096] = {0};
    memset(sql,0,sizeof(sql));
    if(strcmp(targetDir,"")==0){
        sprintf(sql,"select id from file where username = '%s' and tomb = 0 and path is NULL and type = 'D';",
                pCmdShare->username);
    }else{
        sprintf(sql,"select id from file where username = '%s' and tomb = 0 and path = '%s' and type = 'D';",
                pCmdShare->username,targetDir);
    }
    /* printf("sql = %s\n",sql); */
    int qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-1");
        memcpy(train.data,"-1",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    MYSQL_RES* res = mysql_store_result(pCmdShare->mysql);
    /* printf("mysql_num_rows(res) = %ld\n",mysql_num_rows(res)); */
    if(mysql_num_rows(res)<=0){
        dirExist = 0; 
    }
    printf("dirExist = %d\n",dirExist);

    // 目标文件夹不存在，通知客户端
    if(dirExist == 0){
        // printf("target Dir not exist!\n");
        // 通知客户端
        memset(&train,0,sizeof(train_t));
        memcpy(train.data,"-1",strlen("-1"));
        train.length = strlen("-1");
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    //dirExist == 1
    //获取目标目录的id
    MYSQL_ROW row = mysql_fetch_row(res);
    int pId = atoi(row[0]);
    //printf("pId = %d\n",pId);

    //获取目录中的信息
    memset(sql,0,sizeof(sql));
    sprintf(sql,"select type,filename,filesize from file where pre_id = %d and tomb = 0;",pId);
    qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-1");
        memcpy(train.data,"-1",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    memset(res,0,sizeof(MYSQL_RES));
    res = mysql_store_result(pCmdShare->mysql); 
    char fileInfo[4096] = {0};
    while(1){
        memset(row,0,sizeof(MYSQL_ROW));
        row = mysql_fetch_row(res);
        if(row == NULL){
            break;
        }
        memset(fileInfo,0,sizeof(fileInfo));
        sprintf(fileInfo,"%s\t\t%s\t\t\t\t%s",row[0],row[1],row[2]);
        memset(&train,0,sizeof(train_t));
        train.length = strlen(fileInfo);
        memcpy(train.data,fileInfo,train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        //printf("%s\n",fileInfo);
    }

    // 文件信息发送完毕，发送结束标志
    memset(&train,0,sizeof(train_t));
    train.length = strlen("0");
    memcpy(train.data,"0",train.length);
    send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
    send(pCmdShare->netfd,train.data,train.length,0);
    return 0;
}

int serverExecPwd(cmdShare_t* pCmdShare){
    //printf("I am serverExecPwd\n");
    //获取当前路径
    char curPath[4096] = {0};
    getCurPath(&pCmdShare->pathStack,curPath);

    //printf("curPath = %s\n",curPath);
    //发送给客户端
    train_t train;
    memset(&train,0,sizeof(train_t));
    train.length = strlen(curPath);
    memcpy(train.data,curPath,train.length);
    send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
    send(pCmdShare->netfd,train.data,train.length,0);

    return 0;
}

int serverExecPuts(cmdShare_t* pCmdShare,const char* filename){
    //printf("I am serverExecPuts!,filename = %s\n",filename);
    //准备接受的工作
    int stackDeep = 0;
    pathStackNode_t* pCur = pCmdShare->pathStack.pFront;
    while(1){
        if(pCur==NULL){
            break;
        }
        pCur = pCur->pNext;
        stackDeep++;
    }

    train_t train;
    char curWorkDir[4096]={0};
    getCurPath(&pCmdShare->pathStack,curWorkDir);
    //printf("curWorkDir = %s\n",curWorkDir);
    char sql[4096] = {0};
    if(stackDeep==0){
        //如果父目录是根目录
        sprintf(sql,"select id from file where username = '%s' and tomb = 0 and path is NULL and type = 'D';",
                pCmdShare->username);
    }else{
        //父目录不是根目录
        sprintf(sql,"select id from file where username = '%s' and tomb = 0 and path = '%s' and type = 'D';",
                pCmdShare->username,curWorkDir);
    }
    //printf("%s\n",sql);
    int qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-2");
        memcpy(train.data,"-2",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    MYSQL_RES *res = mysql_store_result(pCmdShare->mysql);
    MYSQL_ROW row = mysql_fetch_row(res);
    int preId = atoi(row[0]);
    //printf("preId = %d\n",preId);

    //拼接目标路径
    char targetPath[4096]={0};
    sprintf(targetPath,"%s/%s",curWorkDir,filename);
    //printf("targetPath = %s\n",targetPath);

    //接受失败信息或者sha1码
    memset(&train,0,sizeof(train_t));
    recvn(pCmdShare->netfd,&train.length,sizeof(train.length));
    recvn(pCmdShare->netfd,train.data,train.length);
    if(memcmp(train.data,"-1",train.length)==0){
        return -1;
    }
    //获取sha1码
    char sha1sum[256] = {0};
    memcpy(sha1sum,train.data,train.length);
    //printf("sha1sum = %s\n",sha1sum);

    //查找数据库中是否存在该文件
    sprintf(sql,"select id,filesize from file where sha1sum = '%s';",sha1sum);
    qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-2");
        memcpy(train.data,"-2",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    memset(res,0,sizeof(MYSQL_RES));
    res = mysql_store_result(pCmdShare->mysql);
    if(mysql_num_rows(res)>0){
        //printf("this file is exist!\n");
        MYSQL_ROW row = mysql_fetch_row(res);
        int Id = atoi(row[0]);
        off_t filesize = atol(row[1]);
        //printf("id = %d,filesize = %ld\n",Id,filesize);

        //文件存在通知客户端
        memset(&train,0,sizeof(train_t));
        train.length = strlen("0");
        memcpy(train.data,"0",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);

        //逻辑秒传(以前逻辑删除过一模一样的文件)
        //
        memset(sql,0,sizeof(sql));     
        sprintf(sql,"select id from file where username='%s'and path ='%s' and type = 'F'"
                "and tomb = 1;",
                pCmdShare->username,targetPath);
        qret = mysql_query(pCmdShare->mysql,sql);
        if(qret!=0){
            fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
            memset(&train,0,sizeof(train_t));
            train.length = strlen("-2");
            memcpy(train.data,"-2",train.length);
            send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
            send(pCmdShare->netfd,train.data,train.length,0);
            return -1;
        }
        MYSQL_RES *res = mysql_store_result(pCmdShare->mysql);
        if(mysql_num_rows(res)>0){
            //逻辑上恢复文件
            MYSQL_ROW row = mysql_fetch_row(res);
            int id = atoi(row[0]);
            memset(sql,0,sizeof(sql));
            sprintf(sql,"update file set tomb = 0 where id = %d;",id);
            qret = mysql_query(pCmdShare->mysql,sql);
            if(qret!=0){
                fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
                memset(&train,0,sizeof(train_t));
                train.length = strlen("-2");
                memcpy(train.data,"-2",train.length);
                send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
                send(pCmdShare->netfd,train.data,train.length,0);
                return -1;
            }
        }
        else{
            //不存在以前被删除过一模一样的文件
            //实现秒传
            memset(sql,0,sizeof(sql));     
            sprintf(sql,"insert into file (filename,username,pre_id,path,type,sha1sum,tomb,filesize) "
                    "values ('%s','%s',%d,'%s','F','%s',0,%ld);",
                    filename,pCmdShare->username,preId,targetPath,sha1sum,filesize);
            //printf("%s\n",sql);
            qret = mysql_query(pCmdShare->mysql,sql);
            if(qret!=0){
                fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
                memset(&train,0,sizeof(train_t));
                train.length = strlen("-2");
                memcpy(train.data,"-2",train.length);
                send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
                send(pCmdShare->netfd,train.data,train.length,0);
                return -1;
            }
        }
        //秒传完成，通知客户端
        memset(&train,0,sizeof(train_t));
        train.length = strlen("0");
        memcpy(train.data,"0",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return 0;
    }
    //文件不存在通知客户端
    memset(&train,0,sizeof(train_t));
    train.length = strlen("-1");
    memcpy(train.data,"-1",train.length);
    send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
    send(pCmdShare->netfd,train.data,train.length,0);
    //--------------------------------------------接收文件------------------------------------------
    //接受文件大小
    off_t filesize;
    off_t lastTotal = 0;
    off_t total = 0;
    memset(&train,0,sizeof(train_t));
    recvn(pCmdShare->netfd,&train.length,sizeof(train.length));
    recvn(pCmdShare->netfd,train.data,train.length);
    memcpy(&filesize,(off_t *)train.data,train.length);
    //printf("filesize = %ld\n",filesize);
    int fd = open(sha1sum,O_RDWR|O_CREAT,0666);
    ftruncate(fd,filesize);
    //printf("filesize = %ld,fd = %d\n",filesize,fd);
    while(1){
        char buf[40960] = {0};
        //printf("lastTotal = %ld\n",lastTotal);
        if(filesize - lastTotal >= 40960){
            int ret = recvn(pCmdShare->netfd,buf,40960);
            if(ret == 1){
                //接收失败，通知客户端
                memset(&train,0,sizeof(train_t));
                train.length = strlen("-1");
                memcpy(train.data,"-1",train.length);
                send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
                send(pCmdShare->netfd,&train.data,train.length,0);
                return -1;
            }
            write(fd,buf,40960);
            total += 40960;
        }
        if(filesize - lastTotal == 0){
            break;
        }
        if(filesize - lastTotal < 40960){
            recvn(pCmdShare->netfd,buf,filesize - lastTotal);
            write(fd,buf,filesize - lastTotal);
            total += filesize - lastTotal;
        }
        lastTotal = total;
        //本次接收成功将上传进度发送给客户端
        memset(&train,0,sizeof(train_t));
        char message[1024];
        sprintf(message,"%6.2lf%%",lastTotal*100.0/filesize);
        train.length = strlen(message);
        memcpy(train.data,message,train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
    }
    close(fd);
    //将文件信息插入文件表(file)
    memset(sql,0,sizeof(sql));     
    sprintf(sql,"insert into file (filename,username,pre_id,path,type,sha1sum,tomb,filesize) "
            "values ('%s','%s',%d,'%s','F','%s',0,%ld);",
            filename,pCmdShare->username,preId,targetPath,sha1sum,filesize);
    //printf("%s\n",sql);
    qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-2");
        memcpy(train.data,"-2",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    //传输完成,通知客户端
    memset(&train,0,sizeof(train_t));
    train.length = strlen("0");
    memcpy(train.data,"0",train.length);
    send(pCmdShare->netfd,&train.length,sizeof(train.length),MSG_NOSIGNAL);
    send(pCmdShare->netfd,train.data,train.length,MSG_NOSIGNAL);

    return 0;
}

int serverExecGets(cmdShare_t* pCmdShare,const char* filename){
    //printf("I am serverExecGets,filename = %s\n",filename);
    train_t train;

    //获得目标路径
    char curWorkDir[4096]={0};
    char targetPath[4096]={0};
    getCurPath(&pCmdShare->pathStack,curWorkDir);
    sprintf(targetPath,"%s/%s",curWorkDir,filename);
    //printf("targetPath = %s\n",targetPath);

    //查询是否有目标文件
    char sql[4096]= {0};
    sprintf(sql,"select sha1sum from file where filename='%s' and"
            " username='%s'and path='%s' and type ='F' and tomb = 0;",
            filename,pCmdShare->username,targetPath);
    int qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-2");
        memcpy(train.data,"-2",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    MYSQL_RES* res = mysql_store_result(pCmdShare->mysql);
    if(mysql_num_rows(res)==0){
        //没有目标文件,通知客户端
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-1");
        memcpy(train.data,"-1",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    //获取磁盘文件的名字(sha1sum)
    MYSQL_ROW row = mysql_fetch_row(res);
    char sha1sum[4096]= {0};
    memcpy(sha1sum,row[0],strlen(row[0]));
    //打开磁盘文件
    int fd = open(sha1sum,O_RDWR);
    if(fd == -1){
        //文件打开失败通知客户端
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-1");
        memcpy(train.data,"-1",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    //fd != -1
    struct stat statbuf;
    fstat(fd,&statbuf);
    off_t filesize = statbuf.st_size;
    memset(&train,0,sizeof(train_t));
    train.length = sizeof(off_t);
    memcpy(train.data,&filesize,train.length);
    send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
    send(pCmdShare->netfd,train.data,train.length,0);

    //接收客服端已存在的长度
    off_t existLength;
    memset(&train,0,sizeof(train_t));
    recvn(pCmdShare->netfd,&train.length,sizeof(train.length));
    recvn(pCmdShare->netfd,train.data,train.length);
    memcpy(&existLength,train.data,train.length);
    //printf("existLength = %ld\n",existLength);

    off_t allocateSize = (((filesize)/4096)+1)*4096;
    char* pmmap = (char *)mmap(NULL,allocateSize,PROT_WRITE|PROT_READ,MAP_SHARED,fd,0);
    send(pCmdShare->netfd,pmmap+existLength,filesize-existLength,MSG_NOSIGNAL);
    munmap(pmmap,allocateSize);
    close(fd);

    //接收客户端返回的成功信息
    memset(&train,0,sizeof(train_t));
    recvn(pCmdShare->netfd,&train.length,sizeof(train.length));
    recvn(pCmdShare->netfd,train.data,train.length);
    //printf("train.data = %s\n",train.data);
    if(memcmp(train.data,"0",strlen("0"))==0){
        //printf("Success!\n");
        return 0;
    }
    if(memcmp(train.data,"-1",strlen("-1"))==0){
        //printf("Success!\n");
        return -1;
    }

    return 0;
}

int serverExecRm(cmdShare_t* pCmdShare,const char* filename){
    //printf("I am serverExecRm,filename = %s\n",filename);
    train_t train;

    //获得目标路径
    char curWorkDir[4096]={0};
    char targetPath[4096]={0};
    getCurPath(&pCmdShare->pathStack,curWorkDir);
    sprintf(targetPath,"%s/%s",curWorkDir,filename);
    //printf("targetPath = %s\n",targetPath);

    //查询是否有目标文件
    char sql[4096]= {0};
    sprintf(sql,"select id from file where filename='%s' and"
            " username='%s'and path='%s' and type ='F' and tomb = 0;",
            filename,pCmdShare->username,targetPath);
    int qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-2");
        memcpy(train.data,"-2",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    MYSQL_RES* res = mysql_store_result(pCmdShare->mysql);
    if(mysql_num_rows(res)==0){
        //没有目标文件,通知客户端
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-1");
        memcpy(train.data,"-1",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    //获取要删除文件的id
    MYSQL_ROW row = mysql_fetch_row(res);
    int id = atoi(row[0]);
    //在逻辑上删除此文件
    memset(sql,0,sizeof(sql));
    sprintf(sql,"update file set tomb = 1 where id = %d;",id);
    qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-2");
        memcpy(train.data,"-2",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    //删除成功，通知客户端
    memset(&train,0,sizeof(train_t));
    train.length = strlen("0");
    memcpy(train.data,"0",train.length);
    send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
    send(pCmdShare->netfd,train.data,train.length,0);

    return 0;
}

int serverExecMkdir(cmdShare_t* pCmdShare,const char* dirpath){
    //printf("I am serverExecMkdir,dirpath = %s\n",dirpath);
    char tempDirname[4096]={0};
    memcpy(tempDirname,dirpath,strlen(dirpath));
    int stackDeep = 0;
    pathStackNode_t* pCur = pCmdShare->pathStack.pFront;
    while(1){
        if(pCur==NULL){
            break;
        }
        pCur = pCur->pNext;
        stackDeep++;
    }

    train_t train;
    char curWorkDir[4096]={0};
    getCurPath(&pCmdShare->pathStack,curWorkDir);
    //printf("curWorkDir = %s\n",curWorkDir);
    char sql[4096] = {0};
    if(stackDeep==0){
        //如果父目录是根目录
        sprintf(sql,"select id from file where username = '%s' and tomb = 0 and path is NULL and type = 'D';",
                pCmdShare->username);
    }else{
        //父目录不是根目录
        sprintf(sql,"select id from file where username = '%s' and tomb = 0 and path = '%s' and type = 'D';",
                pCmdShare->username,curWorkDir);
    }
    //printf("%s\n",sql);
    int qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-2");
        memcpy(train.data,"-2",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    MYSQL_RES *res = mysql_store_result(pCmdShare->mysql);
    MYSQL_ROW row = mysql_fetch_row(res);
    int preId = atoi(row[0]);
    //printf("preId = %d\n",preId);
    //判断该目录是否已经存在
    //拼接得到目标文件的路径
    char targetDir[4096]={0};
    sprintf(targetDir,"%s/%s",curWorkDir,tempDirname);
    memset(sql,0,sizeof(sql));
    sprintf(sql,"select* from file where username = '%s' and tomb = 0 and path = '%s' and type = 'D';",
            pCmdShare->username,targetDir);
    qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-2");
        memcpy(train.data,"-2",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    memset(res,0,sizeof(MYSQL_RES));
    res = mysql_store_result(pCmdShare->mysql);
    if(mysql_num_rows(res)>0){
        //该目录已经存在
        printf("this directory is existence!\n");
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-3");
        memcpy(train.data,"-3",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    //查看是否存在逻辑上删除了的一模一样的目录
    memset(sql,0,sizeof(sql));     
    sprintf(sql,"select id from file where username='%s'and path ='%s' and type = 'D'"
            "and tomb = 1;",
            pCmdShare->username,targetDir);
    qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-2");
        memcpy(train.data,"-2",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    memset(res,0,sizeof(MYSQL_RES));
    res = mysql_store_result(pCmdShare->mysql);
    if(mysql_num_rows(res)>0){
        //存在逻辑上删除的了的一模一样的目录
        //逻辑上恢复目录
        MYSQL_ROW row = mysql_fetch_row(res);
        int id = atoi(row[0]);
        memset(sql,0,sizeof(sql));
        sprintf(sql,"update file set tomb = 0 where id = %d;",id);
        qret = mysql_query(pCmdShare->mysql,sql);
        if(qret!=0){
            fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
            memset(&train,0,sizeof(train_t));
            train.length = strlen("-2");
            memcpy(train.data,"-2",train.length);
            send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
            send(pCmdShare->netfd,train.data,train.length,0);
            return -1;
        }
    }
    else{
        //不存在逻辑上删除的了的一模一样的目录
        //创建目录
        memset(sql,0,sizeof(sql));
        sprintf(sql,"insert into file (filename,username,pre_id,path,type,tomb,filesize) values"
                " ('%s','%s',%d,'%s','D',0,4096);",
                tempDirname,pCmdShare->username,preId,targetDir);
        qret = mysql_query(pCmdShare->mysql,sql);
        if(qret!=0){
            fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
            memset(&train,0,sizeof(train_t));
            train.length = strlen("-2");
            memcpy(train.data,"-2",train.length);
            send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
            send(pCmdShare->netfd,train.data,train.length,0);
            return -1;
        }
    }

    //通知客户端创建目录成功
    memset(&train,0,sizeof(train_t));
    train.length = strlen("0");
    memcpy(train.data,"0",train.length);
    send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
    send(pCmdShare->netfd,train.data,train.length,0);

    return 0;
}

int serverExecRmdir(cmdShare_t* pCmdShare,const char* dirpath){
    //printf("I am serverExecRmdir,dirpath = %s\n",dirpath);
    //dirpath中不能包含.和..
    int pathIsIllegal = 0; //1表示非法
    char tempDirpath[4096] = {0};
    memcpy(tempDirpath,dirpath,strlen(dirpath));
    char* arguments[1024];
    char* saveptr;
    arguments[0] = strtok_r(tempDirpath,"/",&saveptr);
    if(memcmp(arguments[0],"..",sizeof(".."))==0){
        pathIsIllegal = 1;
    }
    for(int i=1;i<1024;++i){
        arguments[i] = strtok_r(NULL,"/",&saveptr);
        if(memcmp(arguments[0],"..",sizeof(".."))==0 || memcmp(arguments[0],".",sizeof("."))==0){
            pathIsIllegal = 1;
            break;
        }
        if(arguments[i]==NULL){
            break;
        }
    }
    
    train_t train;
    //路径非法，通知客户端
    if(pathIsIllegal == 1){     
        memset(&train,0,sizeof(train_t));
        memcpy(train.data,"1",sizeof("1"));
        train.length = sizeof(train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    //获取目标目录的路径
    char curWorkDir[4096]={0};
    char targetPath[4096]={0};
    getCurPath(&pCmdShare->pathStack,curWorkDir);
    sprintf(targetPath,"%s/%s",curWorkDir,dirpath);
    //查询是否有目标文件
    char sql[4096]= {0};
    sprintf(sql,"select id from file where filename='%s' and"
            " username='%s'and path='%s' and type ='D' and tomb = 0;",
            dirpath,pCmdShare->username,targetPath);
    int qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-1");
        memcpy(train.data,"-1",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    MYSQL_RES* res = mysql_store_result(pCmdShare->mysql);
    if(mysql_num_rows(res)==0){
        //没有目标文件,通知客户端
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-1");
        memcpy(train.data,"-1",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    //获取要删除目录的id
    MYSQL_ROW row = mysql_fetch_row(res);
    int id = atoi(row[0]);
    //在逻辑上删除此目录
    //删除此目录本身
    memset(sql,0,sizeof(sql));
    sprintf(sql,"update file set tomb = 1 where id = %d;",id);
    qret = mysql_query(pCmdShare->mysql,sql);
    if(qret!=0){
        fprintf(stderr,"mysql_query:%s\n",mysql_error(pCmdShare->mysql));
        memset(&train,0,sizeof(train_t));
        train.length = strlen("-1");
        memcpy(train.data,"-1",train.length);
        send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
        send(pCmdShare->netfd,train.data,train.length,0);
        return -1;
    }
    //递归删除其子目录和文件
    deleteDir(pCmdShare,id);
    //删除成功，通知客户端
    memset(&train,0,sizeof(train_t));
    train.length = strlen("0");
    memcpy(train.data,"0",train.length);
    send(pCmdShare->netfd,&train.length,sizeof(train.length),0);
    send(pCmdShare->netfd,train.data,train.length,0);

    return 0;
}
int deleteDir(cmdShare_t* pCmdShare,int pId){ // 递归的删除文件夹
    //查看pre_id = pId的目录
    char sql[4096] = {0};
    sprintf(sql,"select id,type from file where pre_id = %d and tomb = 0;",pId);
    mysql_query(pCmdShare->mysql,sql);
    MYSQL_RES* res = mysql_store_result(pCmdShare->mysql);
    
//边界条件(目录里没有文件和目录)
    if(mysql_num_rows(res)==0){
        return 0;
    }
//递归公式(逻辑上删除文件和目录)
   MYSQL_ROW row;
   while(1){
       row = mysql_fetch_row(res);
       if(row == NULL){
           break;
       }
       int id = atoi(row[0]);
       //printf("id = %d\n",id);
       if(memcmp(row[2],"D",strlen(row[2]))==0){
           //是文件夹,递归删除
           deleteDir(pCmdShare,id);
       }
       if(memcmp(row[2],"F",strlen(row[2]))==0){
           //是文件,逻辑上删除
           memset(sql,0,sizeof(sql));
           sprintf(sql,"update file set tomb = 1 where id = %d;",id);
           mysql_query(pCmdShare->mysql,sql);
       }
   }
    
    return 0;
}


int getSubpath(const char* pathname,char* subpath){
    char tempPathname[4096] = {0};
    memcpy(tempPathname,pathname,strlen(pathname));
    //切割路径
    char* arguments[1024];
    int arglen;
    arglen = 0;
    char* saveptr;
    arguments[0] = strtok_r(tempPathname,"/",&saveptr);
    if(arguments[0]!=NULL){
        ++arglen;
    }
    for(int i=1;i<1024;++i){
        arguments[i] = strtok_r(NULL,"/",&saveptr);
        if(arguments[i]==NULL){
            break;
        }
        ++arglen;    
    }
    //得到subpath
    if(arglen<=1){
        subpath = NULL;
        return 0;
    }
    int subpathLength = strlen(tempPathname)-strlen(arguments[arglen-1]);
    char subpathname[4096] = {0};
    memcpy(subpathname,tempPathname,subpathLength);
    printf("subpathname = %s\n",subpathname);
    memcpy(subpath,subpathname,strlen(subpathname));
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

