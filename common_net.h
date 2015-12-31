// -------------- Construction des codes de retour --------------------
    // (grossièrement inspiré des codes FTP
    // https://en.wikipedia.org/wiki/List_of_FTP_server_return_codes )

    // On code le tout sur un octet; les deux premiers bits = type de
    // code de retour, le reste désigne le code lui-même
    enum reponse_type {
        REPONSE_OK, // 0
        REPONSE_NOK
    };

    enum ok_code_type {
        ANY_OK,
        ASK_PAGE,
        GIVE_PAGE_OWNER,
        END_WORK
    };

    enum nok_code_type {
        ANY_NOK
    };

    enum code {
        OK_ANY          = ANY_OK          | (REPONSE_OK << 6),
        OK_ASK_PAGE     = ASK_PAGE        | (REPONSE_OK << 6),
        OK_PAGE_OWNER   = GIVE_PAGE_OWNER | (REPONSE_OK << 6),
        OK_END          = END_WORK        | (REPONSE_OK << 6),
        NOK_ANY         = ANY_NOK         | (REPONSE_NOK << 6)
    };
// ---------------------------------------------------------------------

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
bool do_read(int socket, void *output, int taille);

// Pareil que do_read, avec la prise en compte du code de retour
bool do_read_code(int socket, void *output, int taille, enum code* code_ret);

// Envoi d'un message 'input' sur 'socket', de taille 'taille'
void handle_message(int socket, const void *input, int taille);

// Pareil que handle_message, avec une gestion d'un code pr les msg
void message_with_code(int socket, const void *input, int taille, enum code code_ret);
    
// Récupération de la structure d'adresse correspondant à l'ip 'hostname'
struct sockaddr_in* get_addr_info(int port, char* hostname);

// From http://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
/* Get ip from domain name */
int hostname_to_ip(char * hostname , char* ip);

// Connecte & envoi / réception du rang
void connect_and_sr_rank(int socket, struct sockaddr_in serv_add, u_short *rank, u_short self_rank);

// Accepte & réception / envoi du rang. Retourne la socket d'acceptation
int accept_and_rs_rank(int socket, u_short *rank, u_short self_rank);

// Liste les évènements poll survenus
void disp_poll(short revents, int pos);

// Vérifie s'il y a des erreurs sur la socket
// http://stackoverflow.com/questions/4142012/how-to-find-the-socket-connection-state-in-c
void check_sock_err(int socket);
