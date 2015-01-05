#include "mfs.h"
#include "udp.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

int fd;
struct sockaddr_in server;

int UDP_Packet(message* msg, response* res) {
    fd_set set;
    struct timeval timeout; 

    FD_ZERO (&set);
    FD_SET (fd, &set);
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    if ((UDP_Write(fd, &server, (char *)msg, sizeof (message))) == -1){
        printf("Error\n");
        exit(1);
    }

    /*int success = 0;
    while(success == 0){
        int ready = select(fd+1, &set, NULL, NULL, &timeout);
        if(ready == 1){
            if((UDP_Read(fd, &server, (char *)&res, sizeof (response))) == -1){
                printf("Error: No bytes received");
                exit(1);
            }
            success = 1;
        }else{
            printf("Read timeout\n");
            
            FD_ZERO (&set);
            FD_SET (fd, &set);
    
            // Re-initialize the timeout to 5.0 seconds
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;
        }
    }
    */
    if((UDP_Read(fd, &server, (char *) res, sizeof (response))) == -1){
        printf("Error\n");
        exit(1);
    }
    return res->rc;
}
int MFS_Init(char *hostname, int port)
{
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if ((UDP_FillSockAddr(&server, hostname, port)) == -1){
        printf("Error\n");
        exit(1);
    }
    return 0;
}
int MFS_Lookup(int pinum, char *name)
{
    if (strlen(name) > 64) {
        return -1;
    }
    message msg;
    response res;

    strncpy(msg.cmd, "lookup", 24);
    msg.inum = pinum;
    strncpy(msg.name, name, 64);

    res.rc = UDP_Packet(&msg, &res);

    return res.rc;
}
int MFS_Stat(int inum, MFS_Stat_t *m)
{
    message msg;
    response res;

    strncpy(msg.cmd, "stat", 24);
    msg.inum = inum;

    res.rc = UDP_Packet(&msg, &res);

    memcpy(m,&res.stat,sizeof(MFS_Stat_t));
    m = &res.stat;

    return res.rc;
}
int MFS_Write(int inum, char *buffer, int block)
{
    message msg;
    response res;
    strncpy(msg.cmd, "write", 24);

    msg.inum = inum;
    memcpy(msg.block, buffer, sizeof(char[4096]));
    msg.blocknum = block;

    res.rc = UDP_Packet(&msg, &res);

    return res.rc;
}
int MFS_Read(int inum, char *buffer, int block)
{
    message msg;
    response res;

    strncpy(msg.cmd, "read", 24);
    msg.inum = inum;
    msg.blocknum = block;

    res.rc = UDP_Packet(&msg, &res);

    memcpy(buffer, res.block, sizeof(res.block));

    return res.rc;
}
int MFS_Creat(int pinum, int type, char *name)
{
    if (strlen(name) > 64) {
        return -1;
    }
    message msg;
    response res;

    strncpy(msg.cmd, "create", 24);
    msg.type = type;
    msg.inum = pinum;
    strncpy(msg.name, name, 64);

    res.rc = UDP_Packet(&msg, &res);

    return res.rc;
}
int MFS_Unlink(int pinum, char *name)
{
    message msg;
    response res;

    if (strlen(name) > 64) {
        return -1;
    }

    strncpy(msg.cmd, "unlink", 24);
    msg.inum = pinum;
    strncpy(msg.name, name, 64);

    res.rc = UDP_Packet(&msg, &res);

    return res.rc;
}
int MFS_Shutdown()
{
    message msg;
    response res;

    strncpy(msg.cmd, "shutdown", 24);

    res.rc = UDP_Packet(&msg, &res);

    return 0;
}