#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_BUFFER 512
#define STANDARD_PORT 58065
#define USER "99999"
#define PASS "zzzzzzzz"

struct hostent *hostptr;
struct sockaddr_in serveraddr;

int fd, interact = 1;
int cs_port = 0;
char cs_name[MAX_BUFFER], login_user[6], login_pass[9];

void read_n(char *msg, int nbytes);
void read_msg(char *msg);
void read_args(int argc, char *argv[]);
int get_argument_type(char *arg);
int connect_cs();
void disconnect_cs();
void perform_action(char *action, char *action_args);
int login(char *user, char *pass);
void logout();
void dirlist();
void leave();

int main(int argc, char *argv[]) {

  memset(cs_name, '\0', sizeof(cs_name));
  memset(login_user, '\0', sizeof(login_user));
  memset(login_pass, '\0', sizeof(login_pass));

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

  char command[MAX_BUFFER];
  char *action;
  char *action_args;
  char *usr;

  while (interact) {
    if (login_user[0] == '\0')
      usr = "Guest";
    else
      usr = login_user;
    printf("%s:\n > ", usr);
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

void read_n(char *msg, int nbytes) {
  int nr, nleft;
  char *ptr;

  memset(msg, '\0', nbytes);
  nleft = nbytes;
  ptr = msg;
  while (nleft > 0) {
    nr = read(fd,ptr,nleft);
    printf("Number: %d\n", nr);
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
  printf("Read: %s\n", msg);
}

void read_msg(char *msg) {
  char buffer[2];
  while (1) {
    read_n(buffer, 1);
    if (!strcmp(buffer, "\n")) {
      break;
    }
    strcat(msg, buffer);
  }
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

int connect_cs() {

  printf("Connecting to %s:%d\n", cs_name, cs_port);

  fd = socket(AF_INET,SOCK_STREAM,0);
  hostptr = gethostbyname(cs_name);

  memset((void*) &serveraddr, (int) '\0', sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = ((struct in_addr *) (hostptr->h_addr_list[0]))->s_addr;
  serveraddr.sin_port = htons((u_short) cs_port);

  if (connect(fd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) == -1) {
    printf("Failed to connect\n");
    return 0;
  }

  printf("Connected to server\n");
  return 1;
}

void disconnect_cs() {
  close(fd);
}

void perform_action(char *action, char *action_args) {
  if (!strcmp(action, "login")) {
    char *user;
    char *pass;

    user = strtok(action_args, " ");
    pass = strtok(NULL, " ");

    if (strlen(user) != 5 || strlen(pass) != 8) {
      printf("Login arguments are incorrect\n");
      return;
    }

    if (!connect_cs())
      return;
    
    if (login(user, pass))
      printf("Login successful\n");
    else
      printf("Failed to login\n");
    disconnect_cs();
  } else if (!strcmp(action, "logout")) {
    logout();
  } else if (!strcmp(action, "exit")) {
    leave();
  } else if (!strcmp(action, "dirlist")) {
    if (!connect_cs())
      return;
    dirlist();
    disconnect_cs();
  } else
    printf("Action not recognized\n");

}

int login(char *user, char *pass) {
  char login_msg[20], msg[5], buffer[MAX_BUFFER];
  
  if (user[0] == '\0' || pass[0] == '\0') {
    return 0;
  } else {
    strcpy(login_msg, "AUT ");
    strcat(login_msg, user);
    strcat(login_msg, " ");
    strcat(login_msg, pass);
    strcat(login_msg, "\n");

    write(fd, login_msg, 20);

    read_n(msg,4);
    if (!strcmp(msg, "AUR ")) {
      memset(buffer, '\0', MAX_BUFFER);
      read_msg(buffer);
      if (!strcmp(buffer, "OK")) {
        if (login_user[0] == '\0' || login_pass[0] == '\0') {
          strcpy(login_user, user);
          strcpy(login_pass, pass);
        }
        return 1;
      }
      else if (!strcmp(buffer, "NOK")) {
        return 0;
      }
      else if (!strcmp(buffer, "NEW")) {
        strcpy(login_user, user);
        strcpy(login_pass, pass);
        return 1;
      } else {
        printf("Unknown response arguments: %s\n", buffer);
        return 0;
      }
    } else {
      printf("Non standard response: %s\n", msg);
      return 0;
    }
  }
}

void logout() {
  memset(login_user, '\0', sizeof(login_user));
  memset(login_pass, '\0', sizeof(login_pass));
  printf("Logged out\n");
}

void dirlist() {
  char *msg, resp[4], number[4], buffer[1];
  
  printf("Reauthorizing user\n");
  
  if (!login(login_user, login_pass)){
    printf("Login needed to list directories\n");
    return;
  }

  if (!connect_cs()) {
    printf("Failed to reconnect\n");
    return;
  }
  
  msg = "LSD\n";
  if (write(fd, msg, 5) <= 0) {
    printf("Error writing\n");
    return;
  }
  printf("Wrote: %s\n", msg);

  read_n(resp,4);
  printf("Response read: %s\n", resp);
  if (!strcmp(resp, "LDR ")) {
    memset(resp, '\0', 4);
    
    printf("Reading number\n");
    while(1) {
      read_n(buffer,1);
      if (!strcmp(buffer, " ")) {
        memset(buffer, '\0', 1);
        free(buffer);
        break;
      }
      strcat(number, buffer);
      memset(buffer, '\0', 1);
      free(buffer);
    }
    printf("%s\n", number);
  } else {
    printf("Non standard response: %s\n", resp);
    memset(resp, '\0', 4);
    free(resp);
    return;
  }
}

void leave() {
  interact = 0;
}
