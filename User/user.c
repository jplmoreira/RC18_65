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

struct hostent *hostptr;
struct sockaddr_in serveraddr;

int interact = 1;
int cs_port = 0;
char cs_name[MAX_BUFFER], login_user[6], login_pass[9];

void read_args(int argc, char *argv[]);
int get_argument_type(char *arg);
void perform_action(char *action, char *action_args);
int login(char *user, char *pass, int l_fd);
void logout();
void dirlist();
void filelist(char *dir);
void restore(char *dir);
void deluser();
void deldir(char *dir);
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
    int cs_fd;

    user = strtok(action_args, " ");
    pass = strtok(NULL, "\n");

    if (strlen(user) != 5 || strlen(pass) != 8) {
      printf("Login arguments are incorrect\n");
      return;
    }

    if ((cs_fd = tcp_connect(cs_name, cs_port)) == -1)
      return;
    
    if (login(user, pass, cs_fd))
      printf("Login successful\n");
    else
      printf("Failed to login\n");
    tcp_disconnect(cs_fd);
  } else if (!strcmp(action, "logout")) {
    logout();
  } else if (!strcmp(action, "exit")) {
    leave();
  } else if (!strcmp(action, "dirlist")) {
    dirlist();
  } else if (!strcmp(action, "filelist")) {
    filelist(action_args);
  } else if (!strcmp(action, "restore")) {
    restore(action_args);
  } else if (!strcmp(action, "deluser")) {
    deluser();
  } else if (!strcmp(action, "delete")) {
    deldir(action_args);
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

    if(!write_n(login_msg, strlen(login_msg), l_fd)) {
      printf("Connection closed by peer");
      return 0;
    }

    memset(msg, '\0', sizeof(msg));
    if (!read_n(msg,4,l_fd)) {
      printf("Connection closed by peer\n");
      return 0;
    }

    if (!strcmp(msg, "AUR ")) {
      memset(buffer, '\0', sizeof(buffer));
      read_msg(buffer, "\n", l_fd);
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
        printf("New user createad\n");
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
  int n, cs_fd;
  
  if ((cs_fd = tcp_connect(cs_name, cs_port)) == -1)
    return;

  if (!login(login_user, login_pass, cs_fd)){
    printf("Login needed to list directories\n");
    tcp_disconnect(cs_fd);
    return;
  }
  
  memset(msg, '\0', sizeof(msg));
  strcpy(msg, "LSD\n");
  if (!write_n(msg, strlen(msg), cs_fd)) {
    printf("Error writing\n");
    tcp_disconnect(cs_fd);
    return;
  }

  memset(resp, '\0', sizeof(resp));
  if (!read_n(resp,4,cs_fd)) {
    printf("Connection closed by peer\n");
    tcp_disconnect(cs_fd);
    return;
  }
  
  if (!strcmp(resp, "LDR ")) {
    
    memset(dirs, '\0', sizeof(dirs));
    strcpy(dirs, "Directories:");
    memset(number, '\0', sizeof(number));
    read_msg(number, " ", cs_fd);
    n = (int) strtol(number, NULL, 10);

    if (n > 0) {
      for (int i = 0; i < n-1; i++) {
        memset(buffer, '\0', sizeof(buffer));
        read_msg(buffer, " ", cs_fd);
        strcat(dirs, "\n - ");
        strcat(dirs, buffer);
      }
      memset(buffer, '\0', sizeof(buffer));
      read_msg(buffer, "\n", cs_fd);
      strcat(dirs, "\n - ");
      strcat(dirs, buffer);
      printf("%s\n", dirs);
    } else {
      printf("%s has no backed up directories\n", login_user);
    }

    tcp_disconnect(cs_fd);
  } else {
    printf("Non standard response: %s\n", resp);
    tcp_disconnect(cs_fd);
    return;
  }
}

void filelist(char *dir) {
  char msg[26], resp[5], number[4];
  char buffer[MAX_BUFFER], bs_ip[MAX_BUFFER], bs_port[MAX_BUFFER], files[MAX_BUFFER];
  int n, cs_fd;
  
  if ((cs_fd = tcp_connect(cs_name, cs_port)) == -1)
    return;

  if (!login(login_user, login_pass, cs_fd)){
    printf("Login needed to list directories\n");
    tcp_disconnect(cs_fd);
    return;
  }

  memset(msg, '\0', sizeof(msg));
  strcpy(msg, "LSF ");
  strcat(msg, dir);
  strcat(msg, "\n");
  if (!write_n(msg, strlen(msg), cs_fd)) {
    printf("Error writing\n");
    tcp_disconnect(cs_fd);
    return;
  }
  
  memset(resp, '\0', sizeof(resp));
  if (!read_n(resp,4,cs_fd)) {
    printf("Connection closed by peer\n");
    tcp_disconnect(cs_fd);
    return;
  }

  if (!strcmp(resp, "LFD ")) {
    memset(bs_ip, '\0', sizeof(bs_ip));
    read_msg(bs_ip, " ", cs_fd);

    memset(bs_port, '\0', sizeof(bs_port));
    read_msg(bs_port, " ", cs_fd);

    memset(number, '\0', sizeof(number));
    read_msg(number, " ", cs_fd);
    n = (int) strtol(number, NULL, 10);

    if (n > 0) {
      strcpy(files, dir);
      strcat(files, " files:");
    
      for (int i = 0; i < n; i++) {
        memset(buffer, '\0', sizeof(buffer));
        read_msg(buffer, " ",cs_fd);
        strcat(files, "\n - ");
        strcat(files, buffer);

        memset(buffer, '\0', sizeof(buffer));
        read_msg(buffer, " ",cs_fd);
        strcat(files, " ");
        strcat(files, buffer);

        memset(buffer, '\0', sizeof(buffer));
        read_msg(buffer, " ",cs_fd);
        strcat(files, " ");
        strcat(files, buffer);

        memset(buffer, '\0', sizeof(buffer));
        if (i < n - 1) {
          read_msg(buffer, " ",cs_fd);
        } else {
          read_msg(buffer, "\n", cs_fd);
        }
        strcat(files, " ");
        strcat(files, buffer);
      }
      printf("%s\n", files);
    } else {
      printf("There are no files in %s\n", dir);
    }
    tcp_disconnect(cs_fd);
  } else {
    printf("Non standard response: %s\n", resp);
    tcp_disconnect(cs_fd);
    return;
  }
}

void restore(char *dir) {
  char msg[26], resp[5], number[4], bs_ip[MAX_BUFFER], bs_port[MAX_BUFFER];
  char file_name[MAX_BUFFER], file_date[MAX_BUFFER], file_time[MAX_BUFFER], file_size[MAX_BUFFER];
  long port;
  int bs_fd, n, cs_fd;
  struct stat st = {0};

  if ((cs_fd = tcp_connect(cs_name, cs_port)) == -1)
    return;

  if (!login(login_user, login_pass, cs_fd)){
    printf("Login needed to list directories\n");
    tcp_disconnect(cs_fd);
    return;
  }

  memset(msg, '\0', sizeof(msg));
  strcpy(msg, "RST ");
  strcat(msg, dir);
  strcat(msg, "\n");
  if (!write_n(msg, strlen(msg), cs_fd)) {
    printf("Error writing\n");
    tcp_disconnect(cs_fd);
    return;
  }

  memset(resp, '\0', sizeof(resp));
  if (!read_n(resp,4,cs_fd)) {
    printf("Connection closed by peer\n");
    tcp_disconnect(cs_fd);
    return;
  }

  if (!strcmp(resp, "RSR ")) {
    memset(bs_ip, '\0', sizeof(bs_ip));
    read_msg(bs_ip, " ", cs_fd);
    memset(bs_port, '\0', sizeof(bs_port));
    read_msg(bs_port, "\n",cs_fd);
  } else {
    printf("Non standard response: %s\n", resp);
    tcp_disconnect(cs_fd);
    return;
  }

  tcp_disconnect(cs_fd);

  port = strtol(bs_port, NULL, 10);
  
  if ((bs_fd = tcp_connect(bs_ip, port)) == -1)
    return;

  if (!login(login_user, login_pass, bs_fd)){
    printf("Login needed to list directories\n");
    tcp_disconnect(bs_fd);
    return;
  }

  memset(msg, '\0', sizeof(msg));
  strcpy(msg, "RSB ");
  strcat(msg, dir);
  strcat(msg, "\n");
  if (!write_n(msg, strlen(msg), bs_fd)) {
    printf("Error writing\n");
    tcp_disconnect(bs_fd);
    return;
  }

  memset(resp, '\0', sizeof(resp));
  if (!read_n(resp,4,bs_fd)) {
    printf("Connection closed by peer\n");
    tcp_disconnect(bs_fd);
    return;
  }

  if (!strcmp(resp, "RBR ")) {
    if (stat(dir, &st) == -1) {
      mkdir(dir, 0700);
    }

    memset(number, '\0', sizeof(number));
    read_msg(number, " ",bs_fd);
    n = (int) strtol(number, NULL, 10);

    if (n > 0) {
      char name[MAX_BUFFER];
      long size;
      FILE *fp;
      void *buffer;
      for (int i = 0; i < n; i++) {
        memset(file_name, '\0', sizeof(file_name));
        read_msg(file_name," ",bs_fd);
        memset(file_date, '\0', sizeof(file_date));
        read_msg(file_date, " ", bs_fd);
        memset(file_time, '\0', sizeof(file_time));
        read_msg(file_time, " ", bs_fd);
        memset(file_size, '\0', sizeof(file_size));
        read_msg(file_size, " ", bs_fd);

        memset(name, '\0', sizeof(name));
        strcpy(name, dir);
        strcat(name, "/");
        strcat(name, file_name);
        fp = fopen(name, "w");

        size = strtol(file_size, NULL, 10);
        buffer = malloc(size);
        
        if (!read_n(buffer, size, bs_fd)) {
          printf("Connection closed by peer\n");
          fclose(fp);
          tcp_disconnect(bs_fd);
          return;
        }
        fwrite(buffer, size, 1, fp);
        fclose(fp);
        free(buffer);

        if (i < n-1) {
          read_msg(msg, " ", bs_fd);
        }
      }
    } else {
      printf("There are no files in %s to restore\n", dir);
    }
    tcp_disconnect(bs_fd);
  } else {
    printf("Non standard response: %s\n", resp);
    tcp_disconnect(bs_fd);
    return;
  }
}

void deluser() {
  char msg[5], resp[5];
  int cs_fd;

  if ((cs_fd = tcp_connect(cs_name, cs_port)) == -1)
    return;

  if (!login(login_user, login_pass, cs_fd)){
    printf("Login needed to list directories\n");
    return;
  }

  memset(msg, '\0', sizeof(msg));
  strcpy(msg, "DLU\n");
  if (!write_n(msg, strlen(msg), cs_fd)) {
    printf("Error writing\n");
    return;
  }

  memset(resp, '\0', sizeof(resp));
  if (!read_n(resp,4,cs_fd)) {
    printf("Connection closed by peer\n");
    return;
  }

  if (!strcmp(resp, "DLR ")) {
    memset(resp, '\0', sizeof(resp));
    read_msg(resp, "\n", cs_fd);
    if (!strcmp(resp, "OK")) {
      printf("User deleted\n");
      logout();
    }
    else if (!strcmp(resp, "NOK"))
      printf("User still has information stored\n");
    else
      printf("Status not recognized\n");

    tcp_disconnect(cs_fd);
  } else {
    printf("Non standard response: %s\n", resp);
    tcp_disconnect(cs_fd);
    return;
  }
}

void deldir(char *dir) {
  char resp[5], msg[25];
  int cs_fd;

  if ((cs_fd = tcp_connect(cs_name, cs_port)) == -1)
    return;

  if (!login(login_user, login_pass, cs_fd)){
    printf("Login needed to list directories\n");
    tcp_disconnect(cs_fd);
    return;
  }

  memset(msg, '\0', sizeof(msg));
  strcpy(msg, "DLU ");
  strcat(msg, dir);
  if (!write_n(msg, strlen(msg), cs_fd)) {
    printf("Error writing\n");
    tcp_disconnect(cs_fd);
    return;
  }

  memset(resp, '\0', sizeof(resp));
  if (!read_n(resp,4,cs_fd)) {
    printf("Connection closed by peer\n");
    tcp_disconnect(cs_fd);
    return;
  }

  if (!strcmp(resp, "DDR ")) {
    memset(resp, '\0', sizeof(resp));
    read_msg(resp, "\n", cs_fd);
    if (!strcmp(resp, "OK"))
      printf("Directory deleted\n");
    else if (!strcmp(resp, "NOK"))
      printf("Unable to delete %s\n", dir);
    else
      printf("Status not recognized\n");

    tcp_disconnect(cs_fd);
  } else {
    printf("Non standard response: %s\n", resp);
    tcp_disconnect(cs_fd);
    return;
  }
}

void leave() {
  interact = 0;
}
