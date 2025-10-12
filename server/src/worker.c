#include"worker.h"
#include"transMsg.h"
int business(int netfd,MYSQL* mysql){
    train_t train;
    char command[4096] = {0};

    char username[1024]= {0};
    //接收客户端要进行的操作
    memset(&train,0,sizeof(train_t));
    recvn(netfd,&train.length,sizeof(train.length));
    recvn(netfd,train.data,train.length);
    if(memcmp(train.data,"1",train.length)==0){
        //进行注册
        userRegister(netfd,mysql,username);
        SYSTEM_LOG(LOG_NOTICE,username,"Connection successful");
    }else if(memcmp(train.data,"2",train.length)==0){
        //进行登录
        serverPwdAuth(netfd,mysql,username);
        /* printf("pwdAuth username = %s\n",username); */
        SYSTEM_LOG(LOG_NOTICE,username,"Registration successful");
    }

    //定义并初始化共享变量
    cmdShare_t cmdShare;
    cmdShareInit(&cmdShare,netfd,mysql,username);

    while(1){
        int exitFlag = 0;
        //printf("before recvCommand netfd = %d\n",netfd);
        int ret = recvCommand(netfd,command);
        SYSTEM_LOG(LOG_NOTICE,username,command);
        if(ret == -1){
            printf("1 client left!\n");
            break;
        }
        //printf("command =%s\n",command);
        exitFlag = serverExecCommand(&cmdShare,command);
        if(exitFlag == 1){
            break;
        }
        SYSTEM_LOG(LOG_NOTICE,username,"Operation successful");
    }
    SYSTEM_LOG(LOG_NOTICE,username,"Disconnection");
    return 0;
}

int userRegister(int netfd,MYSQL* mysql,char* username){
    train_t train;
    while(1){
        //接收用户名
        char recveduser[256] = {0};
        memset(&train,0,sizeof(train_t));
        recvn(netfd,&train.length,sizeof(train.length));
        recvn(netfd,train.data,train.length);
        memcpy(recveduser,train.data,train.length);
        memcpy(username,recveduser,train.length);
        
        //查询该用户名是否存在
        char sql[4096] = {0};
        int usernameExist = 0;
        sprintf(sql,"select * from user where username = '%s';",username);
        int qret = mysql_query(mysql,sql);
        if(qret != 0){
            //printf("qret = %d\n",qret);
            fprintf(stderr,"mysql_query:%s\n",mysql_error(mysql));
            memset(&train,0,sizeof(train_t));
            train.length = strlen("-1");
            memcpy(train.data,"-1",train.length);
            send(netfd,&train.length,sizeof(train.length),0);
            send(netfd,train.data,train.length,0);
            return -1;
        }
        MYSQL_RES *res = mysql_store_result(mysql); //把row存起来
        if(mysql_num_rows(res)>0){
            usernameExist = 1;
        }
        //通知客户端用户名是否存在
        if(usernameExist == 1){
            //printf("username is exist!\n");
            memset(&train,0,sizeof(train_t));
            train.length = strlen("-1");
            memcpy(train.data,"-1",train.length);
            send(netfd,&train.length,sizeof(train.length),0);
            send(netfd,train.data,train.length,0);
            return -1;
        }else {
            //printf("username is not exist!\n");
            memset(&train,0,sizeof(train_t));
            train.length = strlen("0");
            memcpy(train.data,"0",train.length);
            send(netfd,&train.length,sizeof(train.length),0);
            send(netfd,train.data,train.length,0);
        }
        
        //等待客户端两次密码是否一致
        memset(&train,0,sizeof(train_t));
        recvn(netfd,&train.length,sizeof(train.length));
        recvn(netfd,train.data,train.length);
        if(memcmp(train.data,"-1",train.length)==0){
            //printf("password is deference!\n");
            return -1;
        }

        //用户名不存在，可以注册
        //随机生成盐值(salt)
        char salt[21]={0};
        generateSalt(salt,20);
        //printf("salt = %s\n",salt);

        //将盐值发送给客户端
        memset(&train,0,sizeof(train_t));
        train.length = strlen(salt);
        memcpy(train.data,salt,train.length);
        send(netfd,&train.length,sizeof(train.length),0);
        send(netfd,train.data,train.length,0);
        
        //接收客户端返回的密文密码(secret)
        char secret[1024] = {0};
        memset(&train,0,sizeof(train_t));
        recvn(netfd,&train.length,sizeof(train.length));
        recvn(netfd,train.data,train.length);
        //printf("after recv secret,trian.data = %s\n",train.data);
        memcpy(secret,train.data,train.length);

        //printf("secret = %s\n",secret);
        //将新用户插入用户表
        memset(sql,0,sizeof(sql));
        sprintf(sql,"insert into user(username,salt,encryted_password,tomb) values ('%s','%s','%s',0);",
                username,salt,secret);
        //printf("sql = %s\n",sql);
        qret = mysql_query(mysql,sql);
        if(qret!=0){
            //插入表失败；
            memset(&train,0,sizeof(train_t));
            train.length = strlen("-1");
            memcpy(train.data,"-1",train.length);
            send(netfd,&train.length,sizeof(train.length),0);
            send(netfd,train.data,train.length,0);
            return -1;
        }
        //将新用户根目录插入文件表
        memset(sql,0,sizeof(sql));
        sprintf(sql,"insert into file(username,pre_id,type,tomb) values ('%s',-1,'D',0);",
                username);
        //printf("sql = %s\n",sql);
        qret = mysql_query(mysql,sql);
        if(qret!=0){
            //插入表失败；
            memset(&train,0,sizeof(train_t));
            train.length = strlen("-1");
            memcpy(train.data,"-1",train.length);
            send(netfd,&train.length,sizeof(train.length),0);
            send(netfd,train.data,train.length,0);
            return -1;
        }
        //通知客户端注册成功
        memset(&train,0,sizeof(train_t));
        train.length = strlen("0");
        memcpy(train.data,"0",train.length);
        send(netfd,&train.length,sizeof(train.length),0);
        send(netfd,train.data,train.length,0);
        return 0;
    }
    return -1;
}
int serverPwdAuth(int netfd,MYSQL* mysql,char* username){
    train_t train;
    while(1){
        //接收用户名
        char recveduser[256] = {0};
        memset(&train,0,sizeof(train_t));
        recvn(netfd,&train.length,sizeof(train.length));
        recvn(netfd,train.data,train.length);
        memcpy(recveduser,train.data,train.length);
        memcpy(username,recveduser,train.length);

        //printf("train.length = %d,train.data=%s,recveduser =%s\n",train.length,train.data,recveduser);
        
        //查询用户名是否存在
        char sql[4096] = {0};
        int usernameExist = 0;
        sprintf(sql,"select salt,encryted_password from user where username = '%s' and tomb = 0;",username);
        int qret = mysql_query(mysql,sql);
        if(qret != 0){
            printf("qret = %d\n",qret);
            fprintf(stderr,"mysql_query:%s\n",mysql_error(mysql));
            memset(&train,0,sizeof(train_t));
            train.length = strlen("-1");
            memcpy(train.data,"-1",train.length);
            send(netfd,&train.length,sizeof(train.length),0);
            send(netfd,train.data,train.length,0);
            return -1;
        }
        MYSQL_RES *res = mysql_store_result(mysql); //把row存起来
        if(mysql_num_rows(res)>0){
            usernameExist = 1;
        }
        //通知客户端用户名是否存在
        if(usernameExist == 1){
            //printf("username is exist!\n");
            memset(&train,0,sizeof(train_t));
            train.length = strlen("0");
            memcpy(train.data,"0",train.length);
            send(netfd,&train.length,sizeof(train.length),0);
            send(netfd,train.data,train.length,0);
        }else {
            //printf("username is not exist!\n");
            memset(&train,0,sizeof(train_t));
            train.length = strlen("-1");
            memcpy(train.data,"-1",train.length);
            send(netfd,&train.length,sizeof(train.length),0);
            send(netfd,train.data,train.length,0);
            return -1;
        }
        
        //用户名存在
        // 获得盐值(salt)和密文密码(secret)
        char salt[256] = {0};
        char secret[1024] = {0};
        MYSQL_ROW row;
        row = mysql_fetch_row(res);
        //printf("salt = %s,secret = %s\n",row[0],row[1]);
        memcpy(salt,row[0],strlen(row[0]));
        memcpy(secret,row[1],strlen(row[1]));

        //将salt发送给客户端
        memset(&train,0,sizeof(train_t));
        train.length =strlen(salt);
        memcpy(train.data,salt,train.length);
        send(netfd,&train.length,sizeof(train.length),0);
        send(netfd,train.data,train.length,0);
        //接收客户端发来的密文密码
        memset(&train,0,sizeof(train_t));
        recvn(netfd,&train.length,sizeof(train.length));
        recvn(netfd,train.data,train.length);
        //printf("train.length = %d,recvedSecret =%s\n",train.length,train.data);

        //判断密码是否正确
        if(memcmp(secret,train.data,train.length)==0){
            //密码正确，通知客户端
            memset(&train,0,sizeof(train_t));
            train.length = strlen("0");
            memcpy(train.data,"0",train.length);
            send(netfd,&train.length,sizeof(train.length),0);
            send(netfd,train.data,train.length,0);
            return 0;
        }else{
            //密码错误，通知客户端
            memset(&train,0,sizeof(train_t));
            train.length = strlen("-1");
            memcpy(train.data,"-1",train.length);
            send(netfd,&train.length,sizeof(train.length),0);
            send(netfd,train.data,train.length,0);
            return -1;
        }

    }
    return -1;
}


int generateSalt(char* salt,int length){
    char *str = (char*)calloc(1,length);
    if(str == NULL){
        return -1;
    }
    srand(time(NULL));
    int flag;
    str[0] = '$';
    str[1] = '6';
    str[2] = '$';
    str[length-1] = '$';
    for(int i=3;i<length-1;++i){
        flag = rand()%3;
        switch(flag){
            case 0:
                str[i] = 'A'+rand()%26;
                break;
            case 1:
                str[i] = 'a'+rand()%26;
                break;
            case 2:
                str[i] = '0'+rand()%10;
                break;
            default:
                str[i] = 'x';
                break;
        }
    }
    memcpy(salt,str,length);
    free(str);
    return 0;
}

