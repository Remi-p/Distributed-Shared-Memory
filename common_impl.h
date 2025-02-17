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
#include <ctype.h> 
#include <poll.h>
#include <time.h>

#include <ifaddrs.h> // Récupération de l'addr. ip depuis les interfaces

// Couleurs depuis le projet RE216
#include "colors.h"

#include <netinet/in.h>
#include <netinet/tcp.h>

#define VERBOSE 0

#define BUFFER_CHEMIN 1024

// Timeout pour les accepts/connects en secondes
#define TIMEOUT 1

// Taille maximale pour le nom d'une machine
#define NAME_MAX 25

// Nombre de tentative de connexion
#define N_CONNECT 100

/* autres includes (eventuellement) */
#define ERROR_EXIT(str) {perror(str);exit(EXIT_FAILURE);}

// Pr la phase de développement
#define FFLUSH fflush(stdout); fflush(stderr);

typedef char bool;
    #define true 1
    #define false 0
    
/* definition du type des infos */
/* de connexion des processus dsm */
struct dsm_proc_conn  {
   u_short rank;
   int socket;
   u_short port;
   char * machine_name;
   bool shutdown_ready;
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

// Affiche une ligne jusqu'à l'EOF ou une nouvelle ligne
// Puis ajoute une ligne & flush out
bool disp_line(FILE* out, int in);

// Affichage d'erreur
void error(const char *msg);

// Enlève un élément du tableau de processus en fonction de sa position
void remove_from_pos(dsm_proc_t** process, int* nb_process, int i);

// Enlève un élément du tableau de processus
void remove_from_rank(dsm_proc_t** process, int* nb_process, int rank);

// Retourne la socket correspondant au rang
int get_sckt_from_rank(dsm_proc_t* process, int nb_process, int rank);

// Fonction généraliste pour déplacer un tableau en prenant la place
// d'une des cellules
void remove_any(void *array, int length, int size, int pos);

// Segmentation fault
void gdb_stop();
    
/* Retourne un tableau de struct dsm_proc  de taille du nombre de
 * processus contenant le nom de la machine + le rang */
dsm_proc_t* machine_names(char * name_file, int process_nb);

/* compte le nombre de processus à lancer */
int count_process_nb(char * machine_file);

// Retourne true si tous les processus sont prêt à s'auto-détruire
bool shutdown_ready(dsm_proc_t* process, int nb_process);

// Affiche un texte souligné + saute une ligne
void underlined(char *text, ...);

// Liste les informations de connexion liées à un tableau de struct :
void display_connect_info(dsm_proc_t *process, int num_process);

// Compte le nombre de processus à lancer
int count_process_nb(char * machine_file);

// Enregistre le port au processus de rang rank
bool fill_proc_array(dsm_proc_t *proc_array, int num_procs, u_short rank, u_short port);

// Enregistre le numéro de socket au processus de rang rank
bool fill_proc_sckt(dsm_proc_t *proc_array, int num_procs, u_short rank, int sckt);
