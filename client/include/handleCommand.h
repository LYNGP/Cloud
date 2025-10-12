#include"func.h"
#include"transMsg.h"
#ifndef __handleCommand__
int clientExecCommand(int sockfd,const char* command,const char* ip,const char* port,const char* username);
int verdictCommand(int sockfd,const char* command,const char* ip,const char* port,const char* username); //命令合法返回0，1表示命令错误，-1表示参数错误
int sendCommand(int sockfd,const char* command);

int clientExecCd(int sockfd,const char* dirName);
int clientExecLs(int sockfd,const char* dirName);
int clientExecPwd(int sockfd,char * pwd);
int clientExecRm(int sockfd,const char* filename);
int clientExecMkdir(int sockfd,const char* dirpath);
int clientExecRmdir(int sockfd,const char* dirpath);
int clientExecPuts(int sockfd,const char* filename);
int clientExecGets(int sockfd,const char* filename);

int getReminder(int sockfd,const char* username,const char* ip,const char* port);


int generateSha1sum(const char* filename,char* sha1sum,int length); // 对文件生成sha1码;
#endif
