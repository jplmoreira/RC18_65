#include "../utilities.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_BUFFER 512
#define STANDARD_PORT 58065

int cs_port = 0;
struct sockaddr_in serveraddr, clientaddr;
char buffer_tcp[MAX_BUFFER];

void read_args(int argc, char *argv[]);
void check_argument_type(char *arg);

int main(int argc, char *argv[]){

  pid_t pid_TCP, pid_UDP;

  if (argc == 1) {
    printf("No arguments passed\n");
  } else if(argc > 3) {
    printf("Too many arguments passed\n");
    return -1;
  } else {
    read_args(argc,argv);
  }

  if (cs_port == 0) {
    cs_port = STANDARD_PORT;
  }

  pid_TCP = fork();

  if(pid_TCP == 0){

    int user_fd, new_user_fd, clientlen;

    user_fd = socket(AF_INET,SOCK_STREAM,0);

    memset((void*)&serveraddr, (int)'\0', sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((u_short) PORT); //que port meter?

    bind(fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));

    listen(user_fd,20);

    clientlen = sizeof(clientaddr);
    new_user_fd = accept(user_fd,(struct sockaddr*)&clientaddr, &clientlen);

    read(new_user_fd,buffer_tcp,20);




  }

  return 0;
}

void read_args(int argc, char *argv[]){
  if (argc % 2 == 0) {
    printf("Wrong argument numbers: you should disclose the CS server port by using the flag -p\n");
    exit(-1);
  } else if (argc > 1) {
    check_argument_type(argv[1]);
    cs_port = (int) strtol(argv[2], NULL, 10);
    if (cs_port <= 0) {
      printf("Error parsing CS port\n");
      exit(-1);
    }
  }
}

void check_argument_type(char *arg) {
  if (strlen(arg) != 2 || arg[0] != '-') {
    printf("Argument flags should be 2 characters, the first being '-' and the second a letter\n");
    exit(-1);
  } else if(arg[1] != 'p'){
    printf("Flag not recognized\n");
    exit(-1);
  }
}
