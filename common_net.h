int creer_socket(int type, u_short *port_num, char** ip);

int do_socket(int domain, int type, int protocol);
int do_connect(int socket, struct sockaddr_in serv_add);
void do_bind(int socket, struct sockaddr_in* serv_add);
void do_listen(int socket, int max);
int do_accept(int sckt, struct sockaddr* adresse);
bool do_read(int socket, void *output, int taille, enum code* code_ret);
void handle_message(int socket, const void *input, int taille);
struct sockaddr_in* get_addr_info(int port, char* hostname);
int hostname_to_ip(char * hostname , char* ip);

// Connect & send / receive rank
void connect_and_sr_rank(int socket, struct sockaddr_in serv_add, u_short *rank, u_short self_rank);

// Accept & receive / send rank. Retourne la socket d'acceptation
int accept_and_rs_rank(int socket, u_short *rank, u_short self_rank);
	
