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

void read_args(int argc, char *argv[]);
void check_argument_type(char *arg);
void handle_signal();
void getusrs(char usrs[][15]);
void *usr_thread(void *args);
void read_usr(int usr_fd);
void read_action(int usr_fd, char *usr);
int verify_usr(char *usr);
void write_usr(char *usr);
void deluser(char *usr, int usr_fd);

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

  close(cs_fd);

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

void getusrs(char usrs[][15]) {
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

    printf("Connected to: %d\n", clientaddr.sin_addr.s_addr);

    usr_pid = fork();
    if (usr_pid == 0) {
      read_usr(usr_fd);
      close(usr_fd);
      exit(1);
    } else if (usr_pid < 0) {
      printf("Error forking: %s\n", strerror(errno));
      exit(-1);
    }
  }
  return 0;
}

void read_usr(int usr_fd) {
  char msg[5], buffer[MAX_BUFFER], usr[15];
  
  memset(msg, '\0', sizeof(msg));
  if (!read_n(msg, 4, usr_fd)) {
    printf("Connection closed by peer\n");
    return;
  }

  if (!strcmp(msg, "AUT ")) {
    memset(buffer, '\0', sizeof(buffer));
    read_msg(buffer, "\n", usr_fd);
    memset(usr, '\0', sizeof(usr));
    strcpy(usr, buffer);
    int res = verify_usr(buffer);
    memset(buffer, '\0', sizeof(buffer));
    if (res == 1) {
      strcpy(buffer, "AUR OK\n");
      printf("User authorized\n");
    } else if (res == 0) {
      strcpy(buffer, "AUR NEW\n");
      printf("User created\n");
      write_usr(usr);
    } else {
      strcpy(buffer, "AUR NOK\n");
      printf("User not authorized\n");
    }
    if (!write_n(buffer, strlen(buffer), usr_fd)) {
      printf("Connection closed by peer\n");
      return;
    }
    read_action(usr_fd, usr);
  } else {
    printf("Error: Non standard response %s\n", msg);
    memset(buffer, '\0', sizeof(buffer));
    strcpy(buffer, "ERR\n");
    if (!write_n(buffer, strlen(buffer), usr_fd)) {
      printf("Connection closed by peer\n");
    }
  }
}

void read_action(int usr_fd, char *usr) {
  char msg[4], buffer[MAX_BUFFER];

  memset(msg, '\0', sizeof(msg));
  if(!read_n(msg, 3, usr_fd)) {
    printf("Connection closed by peer\n");
    return;
  }

  if (!strcmp(msg, "DLU")) {
    deluser(usr, usr_fd);
  } else {
    printf("Error: Non standard response %s\n", msg);
    memset(buffer, '\0', sizeof(buffer));
    strcpy(buffer, "ERR\n");
    if (!write_n(buffer, strlen(buffer), usr_fd)) {
      printf("Connection closed by peer\n");
    }
  }
}

int verify_usr(char *usr) {
  char *a, *b, bufa[MAX_BUFFER], bufb[MAX_BUFFER];
  char usrs[MAX_BUFFER][15] = {""};

  getusrs(usrs);

  for (int i = 0; i < MAX_BUFFER; i++) {
    if (!strcmp(usr, usrs[i]))
      return 1;
    else if (strlen(usrs[i]) == 14) {
      memset(bufa, '\0', sizeof(bufa));
      strcpy(bufa, usrs[i]);
      a = strtok(bufa, " ");
      memset(bufb, '\0', sizeof(bufb));
      strcpy(bufb, usr);
      b = strtok(bufb, " ");
      if (!strcmp(a, b)) {
        return -1;
      }
    }
  }
  return 0;
}

void write_usr(char *usr) {
  struct stat st = {0};
  int file = 1;
  FILE *fp;

  if (stat("users.txt", &st) == -1)
    file = 0;
  
  if ((fp = fopen("users.txt", "a")) == NULL) {
    printf("Error saving the users: %s\n", strerror(errno));
    exit(-1);
  }

  if (file)
    fprintf(fp, "\n%s", usr);
  else
    fprintf(fp, "%s", usr);
  
  fclose(fp);
}

void deluser(char *usr, int usr_fd) {
  char buffer[MAX_BUFFER], line[MAX_BUFFER];
  FILE *fp;
  int first = 1, deleted = 0;

  memset(buffer, '\0', sizeof(buffer));

  if ((fp = fopen("users.txt", "r")) == NULL) {
    printf("Error opening the users file: %s\n", strerror(errno));
    exit(-1);
  }

  while (fgets(line, 15, fp) != NULL) {
    if (strcmp(usr, line)) {
      if (first) {
        strcpy(buffer, line);
        first = 0;
      }
      else {
        strcat(buffer, "\n");
        strcat(buffer, line);
      }
    } else
      deleted = 1;
    fgets(line, 2, fp);
  }
  fclose(fp);

  if ((fp = fopen("users.txt", "w")) == NULL) {
    printf("Error deleting user: %s\n", strerror(errno));
    exit(-1);
  }

  fprintf(fp, "%s", buffer);  
  fclose(fp);

  memset(buffer, '\0', sizeof(buffer));
  if (deleted)
    strcpy(buffer, "DLR OK\n");
  else
    strcpy(buffer, "DLR NOK\n");
  if (!write_n(buffer, strlen(buffer), usr_fd)) {
    printf("Connection closed by peer\n");
  }
}
