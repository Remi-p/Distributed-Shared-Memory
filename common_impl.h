#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#define VERBOSE 1

/* autres includes (eventuellement) */

#define ERROR_EXIT(str) {perror(str);exit(EXIT_FAILURE);}

/* definition du type des infos */
/* de connexion des processus dsm */
struct dsm_proc_conn  {
   int rank;
   char * machine_name;
   /* a completer */
};
typedef struct dsm_proc_conn dsm_proc_conn_t; 

/* definition du type des infos */
/* d'identification des processus dsm */
struct dsm_proc {   
  pid_t pid;
  dsm_proc_conn_t connect_info;
};
typedef struct dsm_proc dsm_proc_t;

int creer_socket(int type, int *port_num);
int count_process_nb(char * machine_file);
dsm_proc_t* machine_names(char * name_file, int process_nb);
int do_socket(int domain, int type, int protocol);
int do_connect(int socket, struct sockaddr_in serv_add);
void handle_message(int socket, const void *input, int taille);
