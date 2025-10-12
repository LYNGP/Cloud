#include"func.h"
#include"handleCommand.h"
#include"transMsg.h"
#ifndef __business__
#define __business__
int clientbusiness(int sockfd,const char* ip,const char* port);
int userRegister(int sockfd,char* userName);
int pwdAuth(int sockfd,char * userName);
#endif
