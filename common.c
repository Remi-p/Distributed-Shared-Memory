#include "common_impl.h"

// Display informations on a struct sockaddr_in
void addr_verbose(struct sockaddr_in sockaddr, char *env) {
	
	if (env != NULL)
		fprintf(stdout, "%s :\n", env);
		
	fprintf(stdout, "\tPort : %i\n\tAddr : %i\n", sockaddr.sin_port, sockaddr.sin_addr.s_addr);
	
	fprintf(stdout, "\tsin_family : %i\n\n", sockaddr.sin_family);
	
}

void store_ip(char* interface, char ** ip) {
	
	// Pour l'adresse IP à donner aux processus distants, on récupère
	// les adresses des interfaces de la machine
	struct ifaddrs *ifaddr;
	if (getifaddrs(&ifaddr) == -1)
		error("Erreur de récupération de l'adresse IP ");
		
	struct ifaddrs *present = ifaddr;
	do {
		// Pour une raison inconnue, il y a des versions sans le 
		// netmask. Pour trouver la bonne adresse on regarde donc
		// qu'il soit défini.
		if (strcmp(present->ifa_name, interface) == 0
		&& present->ifa_netmask != NULL) {
			*ip = inet_ntoa( ((struct sockaddr_in* ) present->ifa_addr)->sin_addr );
			break;
		}
		present = present->ifa_next;
	} while (present != NULL);
	
	freeifaddrs(ifaddr);
	
	if (*ip == NULL)
		error("IP de la machine non trouvée ");
}

int creer_socket(int prop, u_short *port_num, char** ip) {
	
	struct sockaddr_in *sock_addr;
	socklen_t sock_addrlen;
	int fd = 0;
   
	if (ip != NULL) store_ip("em1", ip);
	
	/* fonction de creation et d'attachement */
	/* d'une nouvelle socket */
	fd = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* renvoie le numero de descripteur */
	/* et modifie le parametre port_num */
	if (ip == NULL)
		sock_addr = get_addr_info(0, NULL); // 0, c-à-d qu'on laisse le système choisir un port
	else
		sock_addr = get_addr_info(0, *ip);
		
	sock_addrlen = sizeof(struct sockaddr_in);

	do_bind(fd, sock_addr);

	getsockname(fd, (struct sockaddr *)sock_addr, &sock_addrlen);
	*port_num = sock_addr->sin_port;

	return fd;
}

// Segmentation fault
void gdb_stop() {	
	// http://stackoverflow.com/questions/18986351/what-is-the-simplest-standard-conform-way-to-produce-a-segfault-in-c
	const char *s = NULL;
	printf( "%c\n", s[0] );
}

// Fonction d'erreur
void error(const char *msg) {
    perror(msg);
    exit(1);
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
	
	int bindtmp;
	// On va essayer d'assigner un port jusqu'à en trouver un disponible
	
	do {
		bindtmp = bind(socket, (struct sockaddr *) serv_add, sizeof(struct sockaddr));
		
		if (bindtmp == -1) {
			if (errno != EADDRINUSE)
				error("Voici l'erreur concernant la création du bind M. ");
			else {
				serv_add->sin_port += 1;
			}
		}
		
	} while (bindtmp < 0);
	
}

/* Fonction do_connect */
/* ask for connexion */
int do_connect(int socket, struct sockaddr_in serv_add) {
	
	int conn = connect(socket, (struct sockaddr *) &serv_add, sizeof(struct sockaddr));
	
	if (conn == -1) {
		error("Voici l'erreur concernant la connexion ");
	}
	
	printf("Connexion réussie.\n\n");
	
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
	
	int ret;
	
	do {
		ret = accept(sckt, adresse, &length_client);
		
		if (ret < 0 && errno != EINTR) {
			error("Voici l'erreur concernant l'acceptation : ");
		}
		
		if (ret == -1 && errno == EINTR) {
			// D'après man 3 accept :
			// EINTR The accept() function was interrupted by a signal
			//		 that was caught before a valid connection arrived.
			
			// Donc on réessaye l'accept, puisque si on est là c'est
			// qu'il devrait y avoir une connexion
		}
		
	} while (ret < 0);
	
	return ret;
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
	// return 0."
	else if (offset == 0) {
		free(msg_received);
		return false;
	}
	
	if (memcpy(output, msg_received, taille) == NULL)
		error("Erreur de recopie dans le buffer ");
	
	free(msg_received);
	return true;
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

// hostname can be NULL, we then use INADDR_ANY
struct sockaddr_in* get_addr_info(int port, char* hostname) {
	
	struct sockaddr_in* sin = calloc(sizeof(char), sizeof(struct sockaddr_in));
	
	sin->sin_family=AF_INET;
	//sin->sin_port=htons(port);
	sin->sin_port=port;
	
	if (hostname == NULL)
		sin->sin_addr.s_addr=INADDR_ANY; //Utiliser loopback
		//sin.sin_addr.s_addr=inet_addr("127.0.0.1");
	else
		sin->sin_addr.s_addr = inet_addr(hostname);
	
	return sin;
}

// From http://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
/* Get ip from domain name */
int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
         
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
     
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
     
    return 1;
}
