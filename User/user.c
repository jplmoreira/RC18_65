#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define USER "67890"
#define PASS "aaaaaaaa"
#define PORT 58011

char *read_n(int fd, int nbytes);

int main() {
  int fd;
  struct hostent *hostptr;
  struct sockaddr_in serveraddr;
  char *msg, *buffer, resp[20];

  fd = socket(AF_INET,SOCK_STREAM,0);
  hostptr = gethostbyname("tejo.tecnico.ulisboa.pt");

  memset((void*) &serveraddr, (int) '\0', sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = ((struct in_addr *) (hostptr->h_addr_list[0]))->s_addr;
  serveraddr.sin_port = htons((u_short) PORT);

  if (connect(fd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) == -1) {
    printf("Failed to connect\n");
    return 1;
  }

  printf("Connected to server\n");

  char usr[20];


  strcpy(usr, "AUT ");
  strcat(usr, USER);
  strcat(usr, " ");
  strcat(usr, PASS);
  strcat(usr, "\n");

  printf("%s", usr);

  write(fd, usr, 19);


  msg = read_n(fd,4);

  if (strcmp(msg, "AUR ") == 0) {

    while (1) {
      buffer = read_n(fd,1);
      strcat(resp, buffer);
      if (!strcmp(buffer, "\n")) {
        break;
      }
    }
    printf("%s\n", resp);
  } else
    printf("Non standard response: %s\n", msg);

  /*while (1) {
    clientlen = sizeof(clientaddr);
    newfd = accept(fd, (struct sockaddr*) &clientaddr, &clientlen);
    printf("Connected to client\n");

    while((n = read(newfd, buffer, 80)) != 0) {
    buffer[n] = '\0';
    printf("Client: %s\n", buffer);
    printf("Message: ");
    fgets(msg, 80, stdin);
    nw = write(newfd, msg, strlen(msg));
    }
    close(newfd);
    }*/
  close(fd);

  return 0;
}

char* read_n(int fd, int nbytes) {
  int nr, nleft;
  char *msg, *ptr;

  nleft = nbytes;
  msg = (char *) malloc(nbytes*sizeof(char));
  ptr = msg;
  while (nleft > 0) {
    nr = read(fd,ptr,nleft);
    if (nr == -1)
      return NULL;
    else if (nr == 0)
      break;
    nleft -= nr;
    ptr += nr;
  }
  return msg;
}
