// Renvoi le numéro de la socket, enregistre le port utilisé dans port_num
int creer_socket(int type, u_short *port_num, char** ip);

// Fonctions fortement inspirées du projet de réseau RE220
// Création d'une socket
int do_socket(int domain, int type, int protocol);

// Demande de connexion à l'adresse passée en paramètre
int do_connect(int socket, struct sockaddr_in serv_add);

// Binding
void do_bind(int socket, struct sockaddr_in* serv_add);

// Ecoute d'un maximum de max machines
void do_listen(int socket, int max);

// Acceptation d'un client
int do_accept(int sckt, struct sockaddr* adresse);

// Read les données envoyées (retourne false si la socket est fermée)
bool do_read(int socket, void *output, int taille, enum code* code_ret);

// Envoi d'un message 'input' sur 'socket', de taille 'taille'
void handle_message(int socket, const void *input, int taille);

// Récupération de la structure d'adresse correspondant à l'ip 'hostname'
struct sockaddr_in* get_addr_info(int port, char* hostname);

// From http://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
/* Get ip from domain name */
int hostname_to_ip(char * hostname , char* ip);

// Connecte & envoi / réception du rang
void connect_and_sr_rank(int socket, struct sockaddr_in serv_add, u_short *rank, u_short self_rank);

// Accepte & réception / envoi du rang. Retourne la socket d'acceptation
int accept_and_rs_rank(int socket, u_short *rank, u_short self_rank);
