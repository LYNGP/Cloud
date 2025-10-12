#include"business.h"
#include<shadow.h>
#include <crypt.h>
int clientbusiness(int sockfd,const char* ip,const char* port){
    train_t train;

    //密码验证
    char username[256] = {0};

    while(1){
        int ret=-1;
        char flag;
        puts("请选择你操作\n1.用户注册，2.用户登录,3.退出");
        while(1){
            ret = -1;
            flag = getchar();
            if(flag == '1'||flag == '2'||flag=='3'){
                break;
            }
        }
        getchar();
        if(flag == '1'){
            memset(&train,0,sizeof(train_t));
            train.length = strlen("1");
            memcpy(train.data,"1",train.length);
            send(sockfd,&train.length,sizeof(train.length),0);
            send(sockfd,train.data,train.length,0);
            ret = userRegister(sockfd,username);
        }else if(flag == '2'){
            memset(&train,0,sizeof(train_t));
            train.length = strlen("2");
            memcpy(train.data,"2",train.length);
            send(sockfd,&train.length,sizeof(train.length),0);
            send(sockfd,train.data,train.length,0);
            ret = pwdAuth(sockfd,username);
        }else if(flag == '3'){
            return -1;
        }
        //printf("ret = %d\n",ret);
        if(ret == 0){
            break;
        }
    }


    //创建储存文件的目录
    mkdir("netDisk",0777);
    //perror("mkdir");

    while(1){
        clientExecCommand(sockfd,"reminder",ip,port,username);
        char command[4096] = {0};
        char ch;
        for(int i=0;i < 4096;i++)
        {
            ch = getchar();
            if(ch == '\n'){
                command[i] = '\0';
                break;
            }
            command[i] = ch;
        }
        //printf("command = %s\n",command);
        if(memcmp(command,"exit",strlen("exit"))==0){
            break;
        }
        clientExecCommand(sockfd,command,ip,port,username);
    }
}

int pwdAuth(int sockfd,char* userName){
    train_t train;
    while(1){
        //接收用户名和密码
        char username[256]={0};
        char password[256]={0};
        printf("Please enter your username:");
        scanf("%s",username);
        getchar();//接收最后的换行符;
        printf("Please enter your password:");
        scanf("%s",password);
        getchar();//接收最后的换行符

        memcpy(userName,username,strlen(username));

        //char username[256] = "test1";
        //char password[256] = "123456";

        //将用户名发送给服务端
        memset(&train,0,sizeof(train_t));
        train.length = strlen(username);
        memcpy(train.data,username,train.length);
        send(sockfd,&train.length,sizeof(train.length),0);
        send(sockfd,&train.data,train.length,0);
        //接收服务端返回的值
        memset(&train,0,sizeof(train_t));
        recvn(sockfd,&train.length,sizeof(train.length));
        recvn(sockfd,train.data,train.length);
        if(memcmp(train.data,"-1",strlen("0"))==0){
            //服务器端没有对应的用户
            printf("ERROR User not exists!\n");
            return -1;
        }
        //服务端有对应的用户，接收到盐值(salt)
        char salt[1024] = {0};
        memset(&train,0,sizeof(train_t));
        recvn(sockfd,&train.length,sizeof(train.length));
        recvn(sockfd,train.data,train.length);
        memcpy(salt,train.data,train.length);
        //printf("salt = %s\n",salt);
        //对salt和明文密码(password)加密,得到密文密码
        char* secret; 
        secret = crypt(password,salt);
        //将密文密码发送给服务端
        memset(&train,0,sizeof(train_t));
        train.length = strlen(secret);
        memcpy(train.data,secret,train.length);
        send(sockfd,&train.length,sizeof(train.length),0);
        send(sockfd,train.data,train.length,0);
        //接收登录是否成功
        memset(&train,0,sizeof(train_t));
        recvn(sockfd,&train.length,sizeof(train.length));
        recvn(sockfd,train.data,train.length);
        if(memcmp(train.data,"-1",train.length)==0){
            //密码验证失败
            printf("ERROR Incorrect username or password!\n");
            return -1;
        }
        if(memcmp(train.data,"0",train.length)==0){
            //密码验证成功
            printf("Success!\n");
            return 0;
        }
    }
    return -1;
}

int userRegister(int sockfd,char *userName){
    train_t train;
    while(1){
        char username[256] = {0};
        char passWord[256] = {0};
        char rePassWord[256] = {0};

        //获取用户名
        printf("Please enter your username:");
        char ch;
        for(int i=0;(ch = getchar())!='\n';++i){
            username[i] = ch;
        }
        //将用户名发送给用户端
        memset(&train,0,sizeof(train_t));
        train.length = strlen(username);
        memcpy(train.data,username,train.length);
        memcpy(userName,username,strlen(username));
        send(sockfd,&train.length,sizeof(train.length),0);
        send(sockfd,train.data,train.length,0);

        //接收是该用户名是否已存在
        memset(&train,0,sizeof(train_t));
        recvn(sockfd,&train.length,sizeof(train.length));
        recvn(sockfd,train.data,train.length);
        if(memcmp(train.data,"-1",train.length)==0){
            //该用户已经存在
            printf("This username is existance!\n");
            return -1;
        }
        
        //该用户名在服务器中不存在
        //获取密码
        printf("Please enter your password:");
        for(int i=0;(ch = getchar())!='\n';++i){
            passWord[i] = ch;
        }
        //再次获取密码
        printf("Please enter your password again:");
        for(int i=0;(ch = getchar())!='\n';++i){
            rePassWord[i] = ch;
        }
        printf("passWord =%s,rePassWord =%s\n",passWord,rePassWord);
        int isSame = 0;
        //判断两次输入的密码是否一致
        //通知服务器密码是否通过
        if(memcmp(passWord,rePassWord,sizeof(passWord))==0){
            isSame = 1;
            memset(&train,0,sizeof(train_t));
            train.length = strlen("0");
            memcpy(train.data,"0",train.length);
            send(sockfd,&train.length,sizeof(train.length),0);
            send(sockfd,train.data,train.length,0);
            printf("isSame = %d\n",isSame);
        }
        else{
            isSame = 0;
            printf("isSame = %d\n",isSame);
            memset(&train,0,sizeof(train_t));
            train.length = strlen("-1");
            memcpy(train.data,"-1",train.length);
            send(sockfd,&train.length,sizeof(train.length),0);
            send(sockfd,train.data,train.length,0);
            return -1;
        }
        

        //接收盐值
        char salt[1024] = {0};
        memset(&train,0,sizeof(train_t));
        recvn(sockfd,&train.length,sizeof(train.length));
        recvn(sockfd,train.data,train.length);
        memcpy(salt,train.data,train.length);
        //printf("salt = %s\n",salt);
        //对salt和明文密码(password)加密,得到密文密码
        char* secret; 
        secret = crypt(passWord,salt);
        //将密文密码发送给服务端
        memset(&train,0,sizeof(train_t));
        train.length = strlen(secret);
        memcpy(train.data,secret,train.length);
        send(sockfd,&train.length,sizeof(train.length),0);
        send(sockfd,train.data,train.length,0);
        //printf("train.length = %d\n,train.data = %s\n",train.length,train.data);

        //接收注册是否成功
        memset(&train,0,sizeof(train_t));
        recvn(sockfd,&train.length,sizeof(train.length));
        recvn(sockfd,train.data,train.length);
        //printf("train.length = %d,train.data = %s\n",train.length,train.data);
        if(memcmp(train.data,"-1",train.length)==0){
            //注册失败
            printf("Failed!\n");
            return -1;
        }
        if(memcmp(train.data,"0",train.length)==0){
            //注册成功
            printf("Success!\n");
            return 0;
        }
    }
    return 0;
}
