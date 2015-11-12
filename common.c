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

// Fonction d'erreur
void error(const char *msg) {
    perror(msg);
    exit(1);
}

// TOASK : Ne vaut-il pas mieux une liste chaînée ?
/* retourne un tableau de struct dsm_proc */
/* de taille du nombre de processus */
/* contenant le nom de la machine + le rang */
dsm_proc_t* machine_names(char * name_file, int process_nb) {
	
	FILE * fichier;
	dsm_proc_t* proc_array = malloc(process_nb * sizeof(struct dsm_proc));
	int i = 0;
	char * machine = (char *) malloc(NAME_MAX * sizeof(char));
	
	if( (fichier = fopen(name_file, "r")) == NULL)
		perror("fopen");
		
	while(fgets(machine, NAME_MAX, fichier) != NULL) {
		
		// On enlève le retour chariot en même temps que l'on copie la chaîne
		proc_array[i].connect_info.machine_name = (char *) malloc(strlen(machine) - 1);
		
		// Ceci remplace le saut de ligne par une fin de ligne, et stop
		// strcpy
		machine[strlen(machine) - 1] = '\0';
		
		strcpy(proc_array[i].connect_info.machine_name, machine);
		
		// On enregistre le rang, car des machines pourront être fermées
		// changer ainsi l'ordre naturel du tableau
		proc_array[i].connect_info.rank = i;
		
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

// Création du bind
void do_bind(int socket, struct sockaddr_in* serv_add) {
	
	int bindtmp = bind(socket, (struct sockaddr *) serv_add, sizeof(struct sockaddr));
	
	if (bindtmp == -1) {
		error("Voici l'erreur concernant la création du bind M. ");
	}
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

// Ecoute d'un maximum de max machines
void do_listen(int socket, int max) {
	
	int listen_return = listen(socket, max);
	
	if (listen_return == -1) {
		error("Voici l'erreur concernant l'écoute ");
	}
}

// Acceptation d'un client
int do_accept(int sckt, struct sockaddr* adresse) {
	
	socklen_t length_client = sizeof(struct sockaddr);
	
	int sckt2 = accept(sckt, adresse, &length_client);
	
	if (sckt2 < 0) {
		error("Voici l'erreur concernant l'acceptation : ");
	}
	//printf("Bienvenue sur le tchat de l'ambiance, vous possédez la socket n°%i\n", sckt2);
	
	return sckt2;
}

// Read les données envoyées
// Retourne false si la socket est fermée
bool do_read(int socket, void *output, int taille, enum code* code_ret) {
	
	// Ici on réceptionnera la chaîne de caractère + le code
	void* msg_received = calloc( taille, 1 );
	
	// Le serveur attend l'envoi des données dans cette fonction (avec recv)
	memset(output,0,taille);
	
	ssize_t offset = 0;

	offset = recv(socket, msg_received + offset, taille - offset, 0);
	
	if (offset < 0)
		error("Erreur de lecture ");
		
	// D'après `man recv` : "If no messages are available to be received
	// and the peer has performed an orderly shutdown, recv() shall
	// return 0." Dans ce cas, output doit être égale à /quit
	else if (offset == 0) {
		free(msg_received);
		return false;
	}
	else {	
		
		free(msg_received);
		return true;
	}
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

//init the self address structure
// hostname can be NULL, we then use INADDR_ANY
struct sockaddr_in* get_addr_info(int port, char* hostname) {
	
	//~ struct sockaddr_in sin;
	//~ memset(&sin, 0, sizeof(struct sockaddr_in));
	struct sockaddr_in* sin = malloc(sizeof(struct sockaddr_in));
	
	sin->sin_family=AF_INET;
	sin->sin_port=htons(port);
	
	if (hostname == NULL)
		sin->sin_addr.s_addr=INADDR_ANY; //Utiliser loopback
		//sin.sin_addr.s_addr=inet_addr("127.0.0.1");
	else
		sin->sin_addr.s_addr = inet_addr(hostname);
		
	//printf("Port : %i / Addr : %i", sin.sin_port, sin.sin_addr.s_addr);
	
	return sin;

}
