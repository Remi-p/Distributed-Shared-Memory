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
#include <netdb.h>
#include <arpa/inet.h>

#include <ifaddrs.h> // Récupération de l'addr. ip depuis les interfaces

// Couleurs depuis le projet RE216
#include "colors.h"

#include <netinet/in.h>
#include <netinet/tcp.h>

#define VERBOSE 1

#define BUFFER_MAX 5

#define DEFAULT_PORT 7777

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
  int stderr;
  int stdout;
  dsm_proc_conn_t connect_info;
};
typedef struct dsm_proc dsm_proc_t;

	typedef char bool;
	#define true 1
	#define false 0

	enum reponse_type {
		REPONSE_OK, // 0
		REPONSE_NOK
	};
	
	enum ok_code_type {
		ANY_OK
	};
	
	enum nok_code_type {
		ANY_NOK
	};
	
	enum code {
		OK_ANY 			= ANY_OK 			| (REPONSE_OK << 6)
	};

// Affichage d'erreur
void error(const char *msg);

int creer_socket(int type, int *port_num);
int count_process_nb(char * machine_file);
dsm_proc_t* machine_names(char * name_file, int process_nb);
int do_socket(int domain, int type, int protocol);
int do_connect(int socket, struct sockaddr_in serv_add);
void do_bind(int socket, struct sockaddr_in* serv_add);
void do_listen(int socket, int max);
int do_accept(int sckt, struct sockaddr* adresse);
bool do_read(int socket, void *output, int taille, enum code* code_ret);
void handle_message(int socket, const void *input, int taille);
struct sockaddr_in* get_addr_info(int port, char* hostname);
