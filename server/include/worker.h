#include "func.h"
#include "handleCommand.h"
#ifndef __worker__
#define __worker__
int business(int netfd, MYSQL *mysql); // 每个工人线程的业务
int userRegister(int netfd, MYSQL *mysql, char *username);
int serverPwdAuth(int netfd, MYSQL *mysql, char *username);
int generateSalt(char *salt, int length); // 随机生成盐值，一般sha1的盐值为20位

#endif
