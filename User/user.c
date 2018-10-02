#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_BUFFER 128
#define STANDARD_PORT 58065
#define USER "99999"
#define PASS "zzzzzzzz"

int fd, interact = 1;
int cs_port = 0;
char cs_name[MAX_BUFFER];

char *read_n(int nbytes);
void read_args(int argc, char *argv[]);
int get_argument_type(char *arg);
void perform_action(char *action, char *action_args);
void login(char *user, char *pass);
void leave();

int main(int argc, char *argv[]) {
  struct hostent *hostptr;
  struct sockaddr_in serveraddr;

  memset(cs_name, '\0', sizeof(cs_name));

  if (argc == 1) {
    printf("No arguments passed\n");
  } else {
    read_args(argc,argv);
  }

  if (cs_port == 0) {
    cs_port = STANDARD_PORT;
  }

  if (cs_name[0] == '\0') {
    if (gethostname(cs_name, sizeof(cs_name)) == -1) {
      printf("Error getting host name\n");
      return -1;
    }
  }

  printf("Connecting to %s:%d\n", cs_name, cs_port);

  fd = socket(AF_INET,SOCK_STREAM,0);
  hostptr = gethostbyname(cs_name);

  memset((void*) &serveraddr, (int) '\0', sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = ((struct in_addr *) (hostptr->h_addr_list[0]))->s_addr;
  serveraddr.sin_port = htons((u_short) cs_port);

  if (connect(fd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) == -1) {
    printf("Failed to connect\n");
    return 1;
  }

  printf("Connected to server\n");

  char command[MAX_BUFFER];
  char *action;
  char *action_args;

  while (interact) {
    printf("Choose your action:\n > ");
    fflush(stdout);

    fgets(command, MAX_BUFFER, stdin);

    action = strtok(command, " \t\n");
    if (action == NULL)
      continue;

    action_args = strtok(NULL, "\n");
    if (action_args == NULL)
      action_args = "\0";

    perform_action(action, action_args);
  }

  return 0;
}

char* read_n(int nbytes) {
  int nr, nleft;
  char *msg, *ptr;

  nleft = nbytes;
  msg = (char *) malloc(nbytes*sizeof(char));
  ptr = msg;
  while (nleft > 0) {
    nr = read(fd,ptr,nleft);
    if (nr == -1) {
      printf("An error ocurred while reading\n");
      exit(-1);
    } else if (nr == 0) {
      printf("Connection was closed by peer\n");
      break;
    }
    nleft -= nr;
    ptr += nr;
  }
  return msg;
}

char* read_msg() {
  char *buffer, *resp;
  resp = (char *) malloc(MAX_BUFFER * sizeof(char));
  memset(resp, '\0', sizeof(*resp));
  while (1) {
    buffer = read_n(1);
    strcat(resp, buffer);
    if (!strcmp(buffer, "\n")) {
      break;
    }
  }
  return resp;
}

void read_args(int argc, char *argv[]) {
  if (argc % 2 == 0) {
    printf("Wrong argument numbers: you should disclose the CS server name and port by using the flags -n and -p respectively\n");
    exit(-1);
  } else if (argc > 1) {
    int arg;
    for (int i = 1; i < argc; i +=2) {
      arg = get_argument_type(argv[i]);
      if (arg == 1) {
        strcpy(cs_name, argv[i + 1]);
      } else if (arg == 2) {
        cs_port = (int) strtol(argv[i + 1], NULL, 10);
        if (cs_port <= 0) {
          printf("Error parsing CS port\n");
          exit(-1);
        }
      } else {
        printf("Flag not recognized\n");
        exit(-1);
      }
    }
  }
}

int get_argument_type(char *arg) {
  if (strlen(arg) != 2 || arg[0] != '-') {
    printf("Argument flags should be 2 characters, the first being '-' and the second a letter\n");
    exit(-1);
  } else {
    switch(arg[1]) {
    case 'n':
      return 1;
      break;
    case 'p':
      return 2;
      break;
    default:
      return 0;
      break;
    }
  }
}

void perform_action(char *action, char *action_args) {
  if (!strcmp(action, "login")) {
    char *user;
    char *pass;

    user = strtok(action_args, " ");
    pass = strtok(NULL, " ");

    if (strlen(user) != 5 || strlen(pass) != 8) {
      printf("Login arguments are incorrect\n");
      exit(-1);
    }

    login(user, pass);
  } else if (!strcmp(action, "exit")) {
    leave();
  }
}

void login(char *user, char *pass) {
  char login_msg[20], *msg;
  
  strcpy(login_msg, "AUT ");
  strcat(login_msg, user);
  strcat(login_msg, " ");
  strcat(login_msg, pass);
  strcat(login_msg, "\n");

  write(fd, login_msg, 20);

  msg = read_n(4);
  if (!strcmp(msg, "AUR ")) {
    free(msg);
    msg = read_msg();
    if (!strcmp(msg, "OK\n"))
      printf("Login successful\n");
    else if (!strcmp(msg, "NOK\n"))
      printf("Login failed\n");
    else if (!strcmp(msg, "NEW\n"))
      printf("User created\n");
    else
      printf("Unknown response arguments\n");
  } else {
    printf("Non standard response: %s\n", msg);
    return;
  }
}

void leave() {
  close(fd);
  interact = 0;
}
