#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

int tcp_connect(char *host_name, int host_port);
void tcp_disconnect(int fd);
int read_n(void *buf, int nbytes, int r_fd);
void read_msg(char *msg, char *end, int r_fd);
int write_n(void *buf, int nbytes, int r_fd);

#endif // _UTILITIES_H_
