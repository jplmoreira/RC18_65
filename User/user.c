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

struct hostent *hostptr;
struct sockaddr_in serveraddr;

int fd, interact = 1;
int cs_port = 0;
char cs_name[MAX_BUFFER], login_user[6], login_pass[9];

int read_n(char *msg, int nbytes, int r_fd);
void read_msg(char *msg, char *end, int r_fd);
void read_args(int argc, char *argv[]);
int get_argument_type(char *arg);
int connect_cs();
void disconnect_cs();
int connect_bs(char *ip, long port);
void disconnect_bs(int bs_fd);
void perform_action(char *action, char *action_args);
int login(char *user, char *pass, int l_fd);
void logout();
void dirlist();
void filelist(char *dir);
void restore(char *dir);
void leave();

int main(int argc, char *argv[]) {

  memset(cs_name, '\0', sizeof(cs_name));
  memset(login_user, '\0', sizeof(login_user));
  memset(login_pass, '\0', sizeof(login_pass));

  if (argc == 1) {
    printf("No arguments passed\n");
  } else if(argc > 5) {
    printf("Too many arguments passed\n");
    return -1;
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
      usr = "guest";
    else
      usr = login_user;
    printf("\nUser#%s\n > ", usr);
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

int read_n(char *msg, int nbytes, int r_fd) {
  int nr, nleft;
  char *ptr;

  nleft = nbytes;
  ptr = msg;
  memset(msg, '\0', nbytes);
  while (nleft > 0) {
    nr = read(r_fd,ptr,nleft);
    if (nr == -1) {
      printf("An error ocurred while reading: %s\n", strerror(errno));
      exit(-1);
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

int connect_bs(char *ip, long port) {
  int fd;
  struct hostent *bs_hostptr;
  struct sockaddr_in bs_serveraddr;


  printf("Connecting to BS %s:%ld\n", ip, port);
  fd = socket(AF_INET,SOCK_STREAM,0);
  bs_hostptr = gethostbyname(ip);

  memset((void*) &bs_serveraddr, (int) '\0', sizeof(bs_serveraddr));
  bs_serveraddr.sin_family = AF_INET;
  bs_serveraddr.sin_addr.s_addr = ((struct in_addr *) (bs_hostptr->h_addr_list[0]))->s_addr;
  bs_serveraddr.sin_port = htons((u_short) port);

  if (connect(fd, (struct sockaddr*) &bs_serveraddr, sizeof(bs_serveraddr)) == -1) {
    printf("Failed to connect to backup\n");
    return -1;
  }

  printf("Connected to backup\n");
  return fd;
}

void disconnect_bs(int bs_fd) {
  close(bs_fd);
}

void perform_action(char *action, char *action_args) {
  if (!strcmp(action, "login")) {
    char *user;
    char *pass;

    user = strtok(action_args, " ");
    pass = strtok(NULL, "\n");

    if (strlen(user) != 5 || strlen(pass) != 8) {
      printf("Login arguments are incorrect\n");
      return;
    }

    if (!connect_cs())
      return;
    
    if (login(user, pass,fd))
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
  } else if (!strcmp(action, "filelist")) {
    if (!connect_cs())
      return;
    filelist(action_args);
    disconnect_cs();
  } else if (!strcmp(action, "restore")) {
    restore(action_args);
  } else
    printf("Action not recognized\n");

}

int login(char *user, char *pass, int l_fd) {
  char login_msg[20], msg[5], buffer[MAX_BUFFER];
  memset(msg, '\0', sizeof(msg));
  
  if (user[0] == '\0' || pass[0] == '\0') {
    return 0;
  } else {
    strcpy(login_msg, "AUT ");
    strcat(login_msg, user);
    strcat(login_msg, " ");
    strcat(login_msg, pass);
    strcat(login_msg, "\n");

    write(fd, login_msg, strlen(login_msg));

    if (!read_n(msg,4,l_fd)) {
      printf("Connection closed by peer\n");
      return 0;
    }
    if (!strcmp(msg, "AUR ")) {
      memset(buffer, '\0', sizeof(buffer));
      read_msg(buffer, "\n",fd);
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
  char msg[5], resp[5], number[4], buffer[21], dirs[MAX_BUFFER];
  int n;
  
  if (!login(login_user, login_pass,fd)){
    printf("Login needed to list directories\n");
    return;
  }
  
  memset(msg, '\0', sizeof(msg));
  strcpy(msg, "LSD\n");
  if (write(fd, msg, 4) <= 0) {
    printf("Error writing\n");
    return;
  }

  memset(resp, '\0', sizeof(resp));
  if (!read_n(resp,4,fd)) {
    printf("Connection closed by peer\n");
    return;
  }
  
  if (!strcmp(resp, "LDR ")) {
    
    memset(dirs, '\0', sizeof(dirs));
    strcpy(dirs, "Directories:");
    memset(number, '\0', sizeof(number));
    read_msg(number, " ",fd);
    n = (int) strtol(number, NULL, 10);
    for (int i = 0; i < n-1; i++) {
      memset(buffer, '\0', sizeof(buffer));
      read_msg(buffer, " ",fd);
      strcat(dirs, "\n - ");
      strcat(dirs, buffer);
    }
    memset(buffer, '\0', sizeof(buffer));
    read_msg(buffer, "\n",fd);
    strcat(dirs, "\n - ");
    strcat(dirs, buffer);
    printf("%s\n", dirs);
  } else {
    printf("Non standard response: %s\n", resp);
    memset(resp, '\0', sizeof(resp));
    return;
  }
}

void filelist(char *dir) {
  char msg[26], resp[5], number[4];
  char buffer[MAX_BUFFER], bs_ip[MAX_BUFFER], bs_port[MAX_BUFFER], files[MAX_BUFFER];
  int n;
  
  if (!login(login_user, login_pass,fd)){
    printf("Login needed to list directories\n");
    return;
  }

  memset(msg, '\0', sizeof(msg));
  strcpy(msg, "LSF ");
  strcat(msg, dir);
  strcat(msg, "\n");
  if (write(fd, msg, strlen(msg)) <= 0) {
    printf("Error writing\n");
    return;
  }
  
  memset(resp, '\0', sizeof(resp));
  if (!read_n(resp,4,fd)) {
    printf("Connection closed by peer\n");
    return;
  }

  if (!strcmp(resp, "LFD ")) {
    memset(bs_ip, '\0', sizeof(bs_ip));
    read_msg(bs_ip, " ",fd);

    memset(bs_port, '\0', sizeof(bs_port));
    read_msg(bs_port, " ",fd);

    memset(number, '\0', sizeof(number));
    read_msg(number, " ",fd);
    n = (int) strtol(number, NULL, 10);

    strcpy(files, dir);
    strcat(files, " files:");
    
    for (int i = 0; i < n-1; i++) {
      memset(buffer, '\0', sizeof(buffer));
      read_msg(buffer, " ",fd);
      strcat(files, "\n - ");
      strcat(files, buffer);

      memset(buffer, '\0', sizeof(buffer));
      read_msg(buffer, " ",fd);
      strcat(files, " ");
      strcat(files, buffer);

      memset(buffer, '\0', sizeof(buffer));
      read_msg(buffer, " ",fd);
      strcat(files, " ");
      strcat(files, buffer);

      memset(buffer, '\0', sizeof(buffer));
      read_msg(buffer, " ",fd);
      strcat(files, " ");
      strcat(files, buffer);
    }

    memset(buffer, '\0', sizeof(buffer));
    read_msg(buffer, " ",fd);
    strcat(files, "\n - ");
    strcat(files, buffer);

    memset(buffer, '\0', sizeof(buffer));
    read_msg(buffer, " ",fd);
    strcat(files, " ");
    strcat(files, buffer);

    memset(buffer, '\0', sizeof(buffer));
    read_msg(buffer, " ",fd);
    strcat(files, " ");
    strcat(files, buffer);

    memset(buffer, '\0', sizeof(buffer));
    read_msg(buffer, "\n",fd);
    strcat(files, " ");
    strcat(files, buffer);

    printf("%s\n", files);
  } else {
    printf("Non standard response: %s\n", resp);
    memset(resp, '\0', sizeof(resp));
    return;
  }
}

void restore(char *dir) {
  char msg[26], resp[5], bs_ip[MAX_BUFFER], bs_port[MAX_BUFFER], buffer[MAX_BUFFER];
  long port;
  int bs_fd;
  struct stat st = {0};

  if (!connect_cs())
    return;

  if (!login(login_user, login_pass, fd)){
    printf("Login needed to list directories\n");
    return;
  }

  memset(msg, '\0', sizeof(msg));
  strcpy(msg, "RST ");
  strcat(msg, dir);
  strcat(msg, "\n");
  if (write(fd, msg, strlen(msg)) <= 0) {
    printf("Error writing\n");
    return;
  }

  memset(resp, '\0', sizeof(resp));
  if (!read_n(resp,4,fd)) {
    printf("Connection closed by peer\n");
    return;
  }

  if (!strcmp(resp, "RSR ")) {
    memset(bs_ip, '\0', sizeof(bs_ip));
    read_msg(bs_ip, " ",fd);
    memset(bs_port, '\0', sizeof(bs_port));
    read_msg(bs_port, "\n",fd);
  } else {
    printf("Non standard response: %s\n", resp);
    memset(resp, '\0', sizeof(resp));
    return;
  }

  disconnect_cs();

  port = strtol(bs_port, NULL, 10);
  
  if ((bs_fd = connect_bs(bs_ip, port)) == -1) {
    return;
  }

  if (!login(login_user, login_pass, bs_fd)){
    printf("Login needed to list directories\n");
    return;
  }

  memset(msg, '\0', sizeof(msg));
  strcpy(msg, "RSB ");
  strcat(msg, dir);
  strcat(msg, "\n");
  if (write(bs_fd, msg, strlen(msg)) <= 0) {
    printf("Error writing\n");
    return;
  }

  memset(resp, '\0', sizeof(resp));
  if (!read_n(resp,4,bs_fd)) {
    printf("Connection closed by peer\n");
    return;
  }

  if (!strcmp(resp, "RBR ")) {
    if (stat(dir, &st) == -1) {
      mkdir(dir, 0700);
    }

    memset(buffer, '\0', sizeof(buffer));
    read_msg(buffer, " ",bs_fd);
    printf("Number: %s\n", buffer);
  } else {
    printf("Non standard response: %s\n", resp);
    memset(resp, '\0', sizeof(resp));
    return;
  }
}

void leave() {
  interact = 0;
}
