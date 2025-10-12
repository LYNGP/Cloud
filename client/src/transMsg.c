#include"transMsg.h"
int recvn(int sockfd,void* buf,long total){
      char *p = (char *)buf;
      long cursize = 0;
      while(cursize < total){
          ssize_t sret = recv(sockfd,p+cursize,total-cursize,0);
          if(sret == 0){ 
              return 1;
          }   
          cursize += sret;
      }   
      return 0;
  }

