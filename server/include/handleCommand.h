#include"func.h"
#include"pathStack.h"
#include"transMsg.h"
#ifndef __handleCommand__
#define __handleCommand__
typedef struct cmdShare_s{
    int netfd;
    MYSQL* mysql;
    char username[1024];
    pathStack_t pathStack;
}cmdShare_t;

int cmdShareInit(cmdShare_t* pCmdShare,int netfd,MYSQL* mysql,char* username);
int recvCommand(int netfd,char* command);

int serverExecCommand(cmdShare_t* pCmdShare,const char* command);
int serverExecCd(cmdShare_t* pCmdShare,const char* dirName);
int serverExecLs(cmdShare_t* pCmdShare,char* dirName);
int serverExecPwd(cmdShare_t* pCmdShare);
int serverExecPuts(cmdShare_t* pCmdShare,const char* filename); //只允许上传到当前目录
int serverExecGets(cmdShare_t* pCmdShare,const char* filename); //只能从当前目录(pwd)下载文件
int serverExecRm(cmdShare_t* pCmdShare,const char* filename);
int serverExecMkdir(cmdShare_t* pCmdShare,const char* dirpath); //只允许在当前目录下创建文件夹

int serverExecRmdir(cmdShare_t* pCmdShare,const char* dirpath);
//dirpath中不能包含.和..

int getSubpath(const char* pathname,char* subpath);
int deleteDir(cmdShare_t* pCmdShare,int pId); // 递归的删除文件夹

int generateSha1sum(const char* filename,char* sha1sum,int length); // 对文件生成sha1码;
#endif
