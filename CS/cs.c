#include "../utilities.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>

#define MAX_BUFFER 512
#define STANDARD_PORT 58065

int port = 0;
int running = 1;
int cs_fd;
char usrs[MAX_BUFFER][MAX_BUFFER] = { "" };

void read_args(int argc, char *argv[]);
void check_argument_type(char *arg);
void handle_signal();
void getusrs();
void saveusrs();
void *usr_thread(void *args);
void read_usr(int cs_fd, int usr_fd);

int main(int argc, char *argv[]){
  pthread_t usr_t;

  signal(SIGINT, handle_signal);

  if (argc == 1) {
    printf("No arguments passed\n");
  } else if(argc > 3) {
    printf("Too many arguments passed\n");
    return -1;
  } else {
    read_args(argc,argv);
  }

  if (port == 0) {
    port = STANDARD_PORT;
  }

  getusrs();
  
  cs_fd = tcp_server(port);

  if (pthread_create(&usr_t, NULL, usr_thread, &cs_fd)) {
    printf("Error creating the thread for user server: %s\n", strerror(errno));
    return -1;
  }

  while(running) {}

  printf("\nCanceling threads\n");

  if (pthread_cancel(usr_t)) {
    printf("Error canceling the thread: %s\n", strerror(errno));
    return -1;
  }

  if (pthread_join(usr_t, NULL)) {
    printf("Error joining the thread: %s\n", strerror(errno));
    return -1;
  }

  /* saveusrs(); */

  return 0;
}

void read_args(int argc, char *argv[]){
  if (argc % 2 == 0) {
    printf("Wrong argument numbers: you should disclose the CS server port by using the flag -p\n");
    exit(-1);
  } else if (argc > 1) {
    check_argument_type(argv[1]);
    port = (int) strtol(argv[2], NULL, 10);
    if (port <= 0) {
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

void handle_signal() {
  running = 0;
}

void getusrs() {
  int i = 0;
  char line[MAX_BUFFER];
  FILE *fp;

  if ((fp = fopen("users.txt", "r")) == NULL) {
    printf("No users saved\n");
    return;
  }
  
  memset(line, '\0', sizeof(line));
  while (fgets(line, 15, fp) != NULL) {
    strcpy(usrs[i], line);
    fgets(line, 2, fp);
    i++;
  }

  fclose(fp);
}

void saveusrs() {
  FILE *fp;
  int first = 0;

  if ((fp = fopen("users.txt", "w")) == NULL) {
    printf("Error saving the users: %s\n", strerror(errno));
    exit(-1);
  }

  for (int i = 0; i < MAX_BUFFER; i++) {
    if (strlen(usrs[i]) == 14) {
      if (first) {
        fprintf(fp, "\n%s", usrs[i]);
      } else {
        first = 1;
        fprintf(fp, "%s", usrs[i]);
      }
    }
  }

  fclose(fp);
}

void *usr_thread() {
  struct sockaddr_in clientaddr;
  unsigned int clientlen = sizeof(clientaddr);
  int usr_fd;
  pid_t usr_pid;
  
  while (running) {
    usr_fd = accept(cs_fd,(struct sockaddr*)&clientaddr, &clientlen);
    if (usr_fd < 0) {
      printf("Error accepting connection: %s\n", strerror(errno));
      return NULL;
    }

    printf("Connected to: %d", clientaddr.sin_addr.s_addr);

    usr_pid = fork();
    if (usr_pid == 0) {
      read_usr(cs_fd, usr_fd);
      exit(1);
    } else if (usr_pid < 0) {
      printf("Error forking: %s\n", strerror(errno));
      exit(-1);
    }
  }
  return 0;
}

void read_usr(int cs_fd, int usr_fd) {
  
}
