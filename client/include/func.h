#ifndef __WD_FUNC_H
#define __WD_FUNC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <shadow.h>
#include <crypt.h>
#include <openssl/sha.h>

#define SIZE(a) (sizeof(a) / sizeof(a[0]))
#define ARGS_CHECK(argc, n)                                       \
    {                                                             \
        if (argc != n)                                            \
        {                                                         \
            fprintf(stderr, "Error: expected %d arguments\n", n); \
            exit(1);                                              \
        }                                                         \
    }

#define ERROR_CHECK(retval, val, msg) \
    {                                 \
        if (retval == val)            \
        {                             \
            perror(msg);              \
            exit(1);                  \
        }                             \
    }

#define THREAD_ERROR_CHECK(ret, msg)                        \
    {                                                       \
        if (ret != 0)                                       \
        {                                                   \
            fprintf(stderr, "%s:%s\n", msg, strerror(ret)); \
        }                                                   \
    }

#endif
