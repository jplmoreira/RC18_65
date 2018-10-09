#include "utilities.h"

int tcp_connect(char *host_name, int host_port) {
  int fd;
  struct hostent *hostptr;
  struct sockaddr_in serveraddr;
  
  printf("Connecting to %s:%d\n", host_name, host_port);

  fd = socket(AF_INET,SOCK_STREAM,0);
  hostptr = gethostbyname(host_name);

  memset((void*) &serveraddr, (int) '\0', sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = ((struct in_addr *) (hostptr->h_addr_list[0]))->s_addr;
  serveraddr.sin_port = htons((u_short) host_port);

  if (connect(fd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) == -1) {
    printf("Failed to connect\n");
    return -1;
  }

  printf("Connected to server\n");
  return fd;
}

void tcp_disconnect(int fd) {
  close(fd);
}

int read_n(void *buf, int nbytes, int r_fd) {
  int nr, nleft;
  char *ptr;

  nleft = nbytes;
  ptr = buf;
  while (nleft > 0) {
    nr = read(r_fd,ptr,nleft);
    if (nr == -1) {
      printf("An error ocurred while reading: %s\n", strerror(errno));
      return 0;
    } else if (nr == 0) {
      return 0;
    }
    nleft -= nr;
    ptr += nr;
  }
  return 1;
}

void read_msg(char *msg, char *end, int r_fd) {
  char buffer[2];
  while (1) {
    memset(buffer, '\0', sizeof(buffer));
    if (!read_n(buffer, 1, r_fd)) {
      printf("Connection closed by peer\n");
      return;
    }

    if (!strcmp(buffer, end)) {
      break;
    } else if (buffer[0] == '\0') {
      break;
    }
    strcat(msg, buffer);
  }
}

int write_n(void *buf, int nbytes, int w_fd) {
  int nw, nleft;
  char *ptr;

  nleft = nbytes;
  ptr = buf;
  while (nleft > 0) {
    nw = write(w_fd,ptr,nleft);
    if (nw <= 0) {
      return 0;
    }
    nleft -= nw;
    ptr += nw;
  }
  return 1;
}
