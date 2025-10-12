#include "handleCommand.h"
#include <openssl/evp.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int verdictCommand(int sockfd, const char* command, const char* ip, const char* port, const char* username) {
    char tempCommand[4096] = {0};
    memcpy(tempCommand, command, strlen(command));
    char* saveptr;
    char* order = strtok_r(tempCommand, " ", &saveptr);
    int arglen = 0;
    char argv[4096] = {0};

    if (order) ++arglen;
    if (saveptr) memcpy(argv, saveptr, strlen(saveptr));

    char* arguments[1024];
    for (int i = 0; i < 1024; ++i) {
        arguments[i] = strtok_r(NULL, " ", &saveptr);
        if (!arguments[i]) break;
        ++arglen;
    }

    if (memcmp("cd", order, strlen(order)) == 0) {
        if (arglen != 2) return -1;
        sendCommand(sockfd, command);
        clientExecCd(sockfd, argv);
        return 0;
    }
    if (memcmp("ls", order, strlen(order)) == 0) {
        if (arglen != 2) return -1;
        sendCommand(sockfd, command);
        clientExecLs(sockfd, argv);
        return 0;
    }
    if (memcmp("pwd", order, strlen(order)) == 0) {
        if (arglen != 1) return -1;
        sendCommand(sockfd, command);
        char pwd[4096] = {0};
        clientExecPwd(sockfd, pwd);
        printf("%s\n", pwd);
        return 0;
    }
    if (memcmp("puts", order, strlen(order)) == 0) {
        if (arglen != 2) return -1;
        sendCommand(sockfd, command);
        clientExecPuts(sockfd, argv);
        return 0;
    }
    if (memcmp("gets", order, strlen(order)) == 0) {
        if (arglen != 2) return -1;
        sendCommand(sockfd, command);
        clientExecGets(sockfd, argv);
        return 0;
    }
    if (memcmp("rm", order, strlen(order)) == 0) {
        if (arglen != 2) return -1;
        sendCommand(sockfd, command);
        clientExecRm(sockfd, argv);
        return 0;
    }
    if (memcmp("mkdir", order, strlen(order)) == 0) {
        if (arglen != 2) return -1;
        sendCommand(sockfd, command);
        clientExecMkdir(sockfd, argv);
        return 0;
    }
    if (memcmp("rmdir", order, strlen(order)) == 0) {
        if (arglen != 2) return -1;
        sendCommand(sockfd, command);
        clientExecRmdir(sockfd, argv);
        return 0;
    }
    if (memcmp("reminder", order, strlen(order)) == 0) {
        if (arglen != 1) return -1;
        sendCommand(sockfd, command);
        getReminder(sockfd, username, ip, port);
        return 0;
    }
    return 1;
}

int clientExecCommand(int sockfd, const char* command, const char* ip, const char* port, const char* username) {
    int ret = verdictCommand(sockfd, command, ip, port, username);
    if (ret == 1) {
        printf("Failed, your command is error!\n");
        return 1;
    }
    if (ret == -1) {
        printf("Failed, your numbers of argument is error!\n");
        return 1;
    }
    return 0;
}

int sendCommand(int sockfd, const char* command) {
    train_t train;
    memset(&train, 0, sizeof(train_t));
    train.length = strlen(command) + 1;
    send(sockfd, &train.length, sizeof(train.length), 0);
    memcpy(train.data, command, train.length);
    send(sockfd, train.data, train.length, 0);
    return 0;
}

int clientExecCd(int sockfd, const char* dirName) {
    train_t train;
    memset(&train, 0, sizeof(train_t));
    recvn(sockfd, &train.length, sizeof(train.length));
    recvn(sockfd, train.data, train.length);
    if (memcmp(train.data, "-1", 2) == 0) {
        printf("cd: %s : No such directory\n", dirName);
        return -1;
    }
    if (memcmp(train.data, "-2", 2) == 0) {
        printf("Failed: Database operation failure\n");
        return -1;
    }
    if (memcmp(train.data, "0", 1) == 0) {
        printf("success!\n");
        return 0;
    }
    return 0;
}

int clientExecLs(int sockfd, const char* dirName) {
    train_t train;
    while (1) {
        memset(&train, 0, sizeof(train_t));
        recvn(sockfd, &train.length, sizeof(train.length));
        recvn(sockfd, train.data, train.length);
        if (memcmp(train.data, "0", 1) == 0) break;
        if (memcmp(train.data, "-1", 2) == 0) {
            printf("ls: cannot access '%s': No such directory\n", dirName);
            break;
        }
        printf("%s\n", train.data);
    }
    return 0;
}

int clientExecPwd(int sockfd, char* pwd) {
    train_t train;
    memset(&train, 0, sizeof(train_t));
    recvn(sockfd, &train.length, sizeof(train.length));
    recvn(sockfd, train.data, train.length);
    memcpy(pwd, train.data, train.length);
    return 0;
}

int clientExecRm(int sockfd, const char* filename) {
    train_t train;
    memset(&train, 0, sizeof(train_t));
    recvn(sockfd, &train.length, sizeof(train.length));
    recvn(sockfd, train.data, train.length);
    if (memcmp(train.data, "-1", 2) == 0) {
        printf("rm: cannot remove '%s': No such file\n", filename);
        return -1;
    }
    if (memcmp(train.data, "-2", 2) == 0) {
        printf("Failed: Database operation failure\n");
        return -1;
    }
    if (memcmp(train.data, "0", 1) == 0) {
        printf("Success\n");
        return 0;
    }
    return 0;
}

int clientExecMkdir(int sockfd, const char* dirpath) {
    train_t train;
    memset(&train, 0, sizeof(train_t));
    recvn(sockfd, &train.length, sizeof(train.length));
    recvn(sockfd, train.data, train.length);
    if (memcmp(train.data, "-1", 2) == 0) {
        printf("Failed: mkdir %s: please check your path\n", dirpath);
        return -1;
    }
    if (memcmp(train.data, "-2", 2) == 0) {
        printf("Failed: Database operation failure\n");
        return -1;
    }
    if (memcmp(train.data, "-3", 2) == 0) {
        printf("Failed: The directory already exists\n");
        return -1;
    }
    if (memcmp(train.data, "0", 1) == 0) {
        printf("Success\n");
        return 0;
    }
    return 0;
}

int clientExecRmdir(int sockfd, const char* dirpath) {
    train_t train;
    memset(&train, 0, sizeof(train_t));
    recvn(sockfd, &train.length, sizeof(train.length));
    recvn(sockfd, train.data, train.length);
    if (memcmp(train.data, "1", 1) == 0) {
        printf("rm: refusing to remove '.' or '..'\n");
        return -1;
    }
    if (memcmp(train.data, "-1", 2) == 0) {
        printf("Failed: please check your path\n");
        return -1;
    }
    if (memcmp(train.data, "0", 1) == 0) {
        printf("Success\n");
        return 0;
    }
    return 0;
}

int clientExecPuts(int sockfd, const char* filename) {
    train_t train;
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        memset(&train, 0, sizeof(train_t));
        train.length = 2;
        memcpy(train.data, "-1", 2);
        send(sockfd, &train.length, sizeof(train.length), 0);
        send(sockfd, train.data, train.length, 0);
        printf("File opening failure\n");
        return -1;
    }

    char sha1sum[256] = {0};
    generateSha1sum(filename, sha1sum, 41);
    printf("sha1sum = %s\n", sha1sum);

    memset(&train, 0, sizeof(train_t));
    train.length = strlen(sha1sum);
    memcpy(train.data, sha1sum, train.length);
    send(sockfd, &train.length, sizeof(train.length), 0);
    send(sockfd, train.data, train.length, 0);

    memset(&train, 0, sizeof(train_t));
    recvn(sockfd, &train.length, sizeof(train.length));
    recvn(sockfd, train.data, train.length);
    if (memcmp(train.data, "0", 1) == 0) {
        memset(&train, 0, sizeof(train_t));
        recvn(sockfd, &train.length, sizeof(train.length));
        recvn(sockfd, train.data, train.length);
        if (memcmp(train.data, "0", 1) == 0) {
            printf("Success!\n");
            return 0;
        } else {
            printf("Failed!\n");
            return -1;
        }
    }
    if (memcmp(train.data, "-1", 2) == 0) {
        printf("Failed!\n");
        return -1;
    }

    struct stat statbuf;
    fstat(fd, &statbuf);
    off_t filesize = statbuf.st_size;
    off_t allocateSize = ((filesize / 4096) + 1) * 4096;
    memset(&train, 0, sizeof(train_t));
    train.length = sizeof(statbuf.st_size);
    memcpy(&train.data, &statbuf.st_size, sizeof(statbuf.st_size));
    send(sockfd, &train.length, sizeof(train.length), 0);
    send(sockfd, train.data, train.length, 0);
    printf("filesize = %ld\n", filesize);

    char* pmmap = (char*)mmap(NULL, allocateSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    send(sockfd, pmmap, filesize, 0);
    munmap(pmmap, allocateSize);

    while (1) {
        memset(&train, 0, sizeof(train_t));
        recvn(sockfd, &train.length, sizeof(train.length));
        recvn(sockfd, train.data, train.length);
        if (memcmp(train.data, "-1", 2) == 0) {
            printf("upload Failed!\n");
            return -1;
        }
        if (memcmp(train.data, "0", 1) == 0) {
            printf("100.00%%\n");
            return 0;
        }
        printf("%s\r", train.data);
        fflush(stdout);
    }
    return 0;
}

int clientExecGets(int sockfd, const char* filename) {
    chdir("netDisk");
    train_t train;

    memset(&train, 0, sizeof(train_t));
    recvn(sockfd, &train.length, sizeof(train.length));
    recvn(sockfd, train.data, train.length);
    if (memcmp(train.data, "-2", 2) == 0) {
        printf("Database operation failure!\n");
        chdir("../");
        return -1;
    }
    if (memcmp(train.data, "-1", 2) == 0) {
        printf("no this file\n");
        chdir("../");
        return -1;
    }

    off_t filesize;
    memcpy(&filesize, train.data, sizeof(off_t));

    off_t lastTotal = 0;
    int fd;
    if (access(filename, F_OK) == 0) {
        fd = open(filename, O_RDWR | O_APPEND);
        lastTotal = lseek(fd, 0, SEEK_END);
    } else {
        fd = open(filename, O_RDWR | O_CREAT, 0666);
    }

    memset(&train, 0, sizeof(train_t));
    train.length = sizeof(lastTotal);
    memcpy(train.data, &lastTotal, sizeof(lastTotal));
    send(sockfd, &train.length, sizeof(train.length), 0);
    send(sockfd, train.data, train.length, 0);

    off_t total = lastTotal;
    while (1) {
        char buf[40960] = {0};
        if (filesize - lastTotal >= 40960) {
            int ret = recvn(sockfd, buf, 40960);
            if (ret == 1) {
                memset(&train, 0, sizeof(train_t));
                train.length = 2;
                memcpy(train.data, "-1", 2);
                send(sockfd, &train.length, sizeof(train.length), 0);
                send(sockfd, train.data, train.length, 0);
                return -1;
            }
            write(fd, buf, 40960);
            total += 40960;
        } else if (filesize - lastTotal == 0) {
            break;
        } else {
            recvn(sockfd, buf, filesize - lastTotal);
            write(fd, buf, filesize - lastTotal);
            total += filesize - lastTotal;
        }
        lastTotal = total;
        printf("%6.2lf%%\r", lastTotal * 100.0 / filesize);
        fflush(stdout);
    }
    close(fd);

    memset(&train, 0, sizeof(train_t));
    train.length = 1;
    memcpy(train.data, "0", 1);
    send(sockfd, &train.length, sizeof(train.length), 0);
    send(sockfd, train.data, train.length, 0);

    chdir("../");
    printf("100.00%%\n");
    return 0;
}

int getReminder(int sockfd, const char* username, const char* ip, const char* port) {
    char pwd[4096] = {0};
    clientExecPwd(sockfd, pwd);

    // 限制每个字段最大长度，确保总长度 < 4096
    char user[256], ip_str[256], port_str[64], pwd_str[1024];
    snprintf(user,   sizeof(user),   "%.255s", username);
    snprintf(ip_str, sizeof(ip_str), "%.255s", ip);
    snprintf(port_str,sizeof(port_str),"%.63s", port);
    snprintf(pwd_str,sizeof(pwd_str),"%.1023s", pwd);

    char reminder[4096];
    snprintf(reminder, sizeof(reminder), "%s@%s:%s:~%s$ ", user, ip_str, port_str, pwd_str);
    printf("%s", reminder);
    return 0;
}

int generateSha1sum(const char* filename, char* sha1sum, int length) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) return -1;

    const EVP_MD* md = EVP_sha1();
    if (EVP_DigestInit_ex(ctx, md, NULL) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    unsigned char buf[4096];
    ssize_t bytes;
    while ((bytes = read(fd, buf, sizeof(buf))) > 0) {
        if (EVP_DigestUpdate(ctx, buf, bytes) != 1) {
            close(fd);
            EVP_MD_CTX_free(ctx);
            return -1;
        }
    }
    close(fd);

    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    if (EVP_DigestFinal_ex(ctx, md_value, &md_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return -1;
    }

    EVP_MD_CTX_free(ctx);

    for (unsigned int i = 0; i < md_len; ++i) {
        snprintf(sha1sum + 2 * i, 3, "%02x", md_value[i]);
    }
    return 0;
}