#include "common_impl.h"
#define NAME_MAX 25
int creer_socket(int prop, int *port_num) 
{
   int fd = 0;
   
   /* fonction de creation et d'attachement */
   /* d'une nouvelle socket */
   /* renvoie le numero de descripteur */
   /* et modifie le parametre port_num */
   
   return fd;
}

/* Vous pouvez ecrire ici toutes les fonctions */
/* qui pourraient etre utilisees par le lanceur */
/* et le processus intermediaire. N'oubliez pas */
/* de declarer le prototype de ces nouvelles */
/* fonctions dans common_impl.h */

/* compte le nombre de processus a lancer */
int count_process_nb(char * machine_file) {
	
	int c;
	unsigned int process_nb = 0;
	FILE * fichier;
	
	if( (fichier = fopen(machine_file, "r")) == NULL)
		return -1;
		
	while( (c = fgetc(fichier)) != EOF ){
		if(c == '\n')
			++process_nb;
	}
	
	return process_nb;
}

/* retourne un tableau de struct dsm_proc */
/* de taille du nombre de processus */
/* contenant le nom de la machine */
dsm_proc_t* machine_names(char * name_file, int process_nb) {
	
	FILE * fichier;
	dsm_proc_t* proc_array = malloc(process_nb * sizeof(*proc_array));
	int i = 0;
	char * machine = (char *) malloc(NAME_MAX);
	
	if( (fichier = fopen(name_file, "r")) == NULL)
		perror("fopen");
		
	while(fgets(machine, NAME_MAX, fichier) != NULL) {
		
		proc_array[i].connect_info.machine_name = (char *)malloc(strlen(machine));
		strcpy(proc_array[i].connect_info.machine_name, machine);;
		i++;
	}
	
	return proc_array;
	
}

/* fonction do_socket */
/* créer une socket */
int do_socket(int domain, int type, int protocol) {
	
    int sockfd;
    int yes = 1;

    // Creation de la socket client
	sockfd = socket(domain, type, protocol);

	// Set socket options
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, 
													sizeof(int)) == -1)
        error("ERROR setting socket options");

    return sockfd;
}

/* fonction do_socket */
/* créer une socket */
int do_socket(int domain, int type, int protocol) {
	
    int sockfd;
    int yes = 1;

    // Creation de la socket client
	sockfd = socket(domain, type, protocol);

	// Set socket options
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, 
													sizeof(int)) == -1)
        error("ERROR setting socket options\n");

    return sockfd;
}

/* foncton do_connect */
/* ask for connexion */
int do_connect(int socket, struct sockaddr_in serv_add) {
	
	int conn = connect(socket, (struct sockaddr *) &serv_add, sizeof(struct sockaddr));
	
	if (conn == -1) {
		error("Voici l'erreur concernant la connexion ");
	}
	
	printf("Connexion réussie (%i), bienvenue sur le tchat.\n\n", conn);
	
	return conn;
}

/* Message sending */
// modification l'input : char * -> const void *
void handle_message(int socket, const void *input, int taille) {
	
	int offset = 0;
	
	do {
		offset = send(socket, input + offset, taille - offset, 0);
		
		if (offset == -1) {
			error("Voici l'erreur concernant l'envoi ");
		}
	}
	while (offset!=taille);
}
