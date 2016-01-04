#include "dsm.h"

int dsm_node_num; /* nombre de processus dsm */
int DSM_NODE_ID;  /* rang (= numero) du processus */ 

bool finalization = false;

/* indique l'adresse de debut de la page de numero numpage */
static char *num2address( int numpage )
{
   char *pointer = (char *)(BASE_ADDR+(numpage*(PAGE_SIZE)));
   
   if( pointer >= (char *)TOP_ADDR ){
      fprintf(stderr,"[%i] Invalid address !\n", DSM_NODE_ID);
      return NULL;
   }
   else return pointer;
}

// Indique le numéro de page correspondant à une adresse
static int address2num(void *addr) {
    
    // (Équation ..)
    int numpage = (int) (addr - BASE_ADDR) / PAGE_SIZE;
    
    if( numpage > PAGE_NUMBER || numpage < 0 ) {
        error("Le numéro de page trouvé est incorrect");
        return -1; // Non exécuté
    }
    else
        return numpage;
}

/* fonctions pouvant etre utiles */
static void dsm_change_info( int numpage, dsm_page_state_t state, dsm_page_owner_t owner)
{
    if ((numpage >= 0) && (numpage < PAGE_NUMBER)) {    
        if (state != NO_CHANGE )
            table_page[numpage].status = state;
        if (owner >= 0 )
            table_page[numpage].owner = owner;
        return;
    }
    else {
        fprintf(stderr,"[%i] Invalid page number !\n", DSM_NODE_ID);
        return;
    }
}

static dsm_page_owner_t get_owner( int numpage) {
   return table_page[numpage].owner;
}

static dsm_page_state_t get_status( int numpage)
{
   return table_page[numpage].status;
}

/* Allocation d'une nouvelle page */
static void dsm_alloc_page( int numpage )
{
   char *page_addr = num2address( numpage );
   mmap(page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   return ;
}

/* Changement de la protection d'une page */
static void dsm_protect_page( int numpage , int prot)
{
   char *page_addr = num2address( numpage );
   mprotect(page_addr, PAGE_SIZE, prot);
   return;
}

static void dsm_free_page( int numpage )
{
   char *page_addr = num2address( numpage );
   munmap(page_addr, PAGE_SIZE);
   return;
}

// Envoi d'une page à la socket renseignée
static void dsm_send(int dest, int page) {
    
    char *page_addr = num2address( page );
    
    void *buf = malloc(PAGE_SIZE + sizeof(int));
    
    // Attribution du numéro de page au début du buffer
    *((int* )buf) = page;
    
    // Copie des données à envoyer
    memcpy(buf + sizeof(int), page_addr, PAGE_SIZE);
    
    // Envoi du buffer
    message_with_code(dest, buf, PAGE_SIZE + sizeof(int), OK_SEND_PAGE);
    
}

static void dsm_recv(void *buf) {
   
    int page_number = *((int *) buf);

    // Décalage pour avoir la page elle même
    char* recv_page = buf + sizeof(int);

    dsm_alloc_page(page_number);

    // Copie des données reçues
    memcpy(num2address(page_number), recv_page, PAGE_SIZE);
    
    // Mise à jour du propriétaire
    dsm_change_info(page_number, WRITE, DSM_NODE_ID);
    
}

// Pareil que dsm_give_owner mais à un processus spécifique
void dsm_give_owner_to(int dest, int pagenumber) {
    
    int infos[2]; // Numéro de page + Propriétaire
    infos[0] = pagenumber;
    infos[1] = get_owner(pagenumber);
    
    message_with_code(dest, infos, sizeof(int)*2, NOK_PAGE_OWNER);
}

// Indique le nouveau propriétaire d'une page
void dsm_give_owner(int pagenumber) {
	
	int i;
    
    int infos[2]; // Numéro de page + Propriétaire
    infos[0] = pagenumber;
    infos[1] = get_owner(pagenumber);
    
	for (i = 0; i < dsm_node_num; i++)
        if (proc_array[i].connect_info.rank != infos[1])
            message_with_code(proc_array[i].connect_info.socket, infos, sizeof(int)*2, OK_PAGE_OWNER);
    
}

static void *dsm_comm_daemon( void *arg) {
    
	// Ici on stockera toutes les sockets des autres processus
    int retour_poll;
    int i;
    int sckt_tmp;
    
    // Stockage des messages reçus
    char output[BUFFER_MAX];
    
    // Stockage des codes reçus
    enum code code_ret;
    
    // Stockage des numéros de page demandés
    int page_number; // OK_ASK_PAGE
    // Stockage des attributions de propriétaires
    int page_owner[2]; // OK_PAGE_OWNER
    // Stockage du n° de page + la page elle-même
    char* recv_page = malloc(PAGE_SIZE + sizeof(int)); // OK_SEND_PAGE
    
    // --------------- Initialisation pour le poll ---------------------
    
    // Necessaire pour l'argument du poll
    int nb_fds = 0;
    
    // Tableau des descripteurs
    struct pollfd fds[dsm_node_num];
		memset(fds, 0, sizeof(struct pollfd) * dsm_node_num);
	
	for (i = 0; i < dsm_node_num; i++) {
		sckt_tmp = proc_array[i].connect_info.socket;
		
		fds[i].fd = sckt_tmp;
		fds[i].events = POLLIN | POLLHUP;
		
		check_sock_err(sckt_tmp);
		
		nb_fds++;
	}
    
    while(shutdown_ready(proc_array, dsm_node_num) == false
    || finalization == false) {
        
        // man poll : If the value of timeout is −1, poll() shall block
        //            until a requested event occurs or until the call
        //            is interrupted.
        while ( (retour_poll = poll( fds, nb_fds, 200)) == -1 ) {
            if ( errno != EINTR )
                error("Erreur lors du select ");
        }
        
        // Si on arrive là, c'est qu'une connexion a été fermée ou qu'un
        // processus nous a envoyé des informations.
        
        for (i = 0; i < dsm_node_num; i++) {
            
            sckt_tmp = proc_array[i].connect_info.socket;
            
            // Debug : display les évènements reçus
            //~ disp_poll(fds[i].revents, i);
            
            // Des informations à lire
            if (fds[i].revents & POLLIN) {
				
				// Remise à zero du buffer
				memset(output, 0, BUFFER_MAX * sizeof(char));
				
				// Lecture
				if (do_read_code(fds[i].fd, NULL, 0, &code_ret) == false)
					if (VERBOSE) fprintf(stdout, "La socket distante a été fermée\n");
				
				//~ // Réaction en fonction du code
                switch (code_ret)
                {
                    case OK_END:
                        //~ // Passage en "prêt à shutdown"
                        proc_array[i].connect_info.shutdown_ready = true;
                        
                        if (VERBOSE) fprintf(stdout, "OK_END par %i\n", proc_array[i].connect_info.rank);
                        
                        break;
                        
                    case OK_ASK_PAGE:
                        if (do_read(fds[i].fd, &page_number, sizeof(int)) == false)
                            break;
                        
                        if (VERBOSE) fprintf(stdout, "On nous demande la page n°%i (par %i)\n", page_number, proc_array[i].connect_info.rank);
                        
                        if (get_owner(page_number) != DSM_NODE_ID) {
                            
                            if (VERBOSE) fprintf(stdout, "Réception d'une demande ne nous concernant pas.\n");
                            
                            // On envoie le propriétaire réel
                            dsm_give_owner_to(proc_array[i].connect_info.socket, page_number);
                        }
                        else {
                            // Changement interne du numéro de propriétaire
                            dsm_change_info(page_number, NO_CHANGE, proc_array[i].connect_info.rank);
                            
                            // Envoie au nouveau proprio. de la page
                            dsm_send(proc_array[i].connect_info.socket, page_number);
                            
                            // Partage du nouveau proprio. aux autres process
                            dsm_give_owner(page_number);
                            
                            // Libération de la page
                            dsm_free_page(page_number);
                        }
                        break;
                        
                    case OK_PAGE_OWNER:
                    case NOK_PAGE_OWNER:
                        if (do_read(fds[i].fd, page_owner, sizeof(int)*2) == false)
                            break;
                            
                        if (VERBOSE) fprintf(stdout, "Attribution du propriétaire de rang %i de la page %i, par %i\n", page_owner[1], page_owner[0], proc_array[i].connect_info.rank);
                        
                        // Mise à jour uniquement si on est pas concerné
                        // (sinon la MaJ se fera avec réception de la
                        // page)
                        if (page_owner[1] != DSM_NODE_ID)
                            dsm_change_info(page_owner[0], NO_CHANGE, page_owner[1]);
                            
                        break;
                        
                    case OK_SEND_PAGE:
                        // Réception d'une page
                        
                        if (do_read(fds[i].fd, recv_page, PAGE_SIZE + sizeof(int)) == false)
                            break;
                            
                        if (VERBOSE) fprintf(stdout, "Réception d'une page envoyée par %i\n", proc_array[i].connect_info.rank);
                        
                        dsm_recv(recv_page);
                        
                        //~ pthread_cond_signal(&wait_page);    
                        
                        break;
                        
                    default:
                        error("Code de retour inconnu (dsm_comm_daemon) ");
                        
                }
                
			}
            
            // Socket fermée
            if (fds[i].revents & POLLHUP) {
                fprintf(stderr, "[%i] Rang %i:\n", DSM_NODE_ID, proc_array[i].connect_info.rank);
				error("Fermeture d'une socket avant l'arret mutualisé ! ");
                //~ remove_from_pos(&proc_array, &dsm_node_num, i);
                
                //~ // Suppression + décrémentation
                //~ remove_any(fds, nb_fds, sizeof(struct pollfd), i);
                //~ nb_fds -= 1;
                
                // /!\ S'il y a un accès à dsm_node_num & co, il faut
                //     protéger ça avec des verrous
			}
        } // Fin du for de parcours de vérification des desc.
        
        FFLUSH
    }
    
    free(recv_page);
    
    pthread_exit(NULL);
	//~ while(1)
	//~ {
	//~ /* a modifier */
	//~ printf("[%i] Waiting for incoming reqs \n", DSM_NODE_ID);
	//~ sleep(2);
	//~ }
   return NULL;
}

static void dsm_handler( int page_number )
{  
    /* A modifier */
    fprintf(stderr, "[%i] Accès interdit sur la page %i de l'utilisateur %i ; envoi d'une demande ... \n", DSM_NODE_ID, page_number, get_owner(page_number));
    
    // Rang du propriétaire de la page
    int rank;
    
    // On boucle, comme ça si le propriétaire change mais que ce n'est
    // toujours pas nous, il y a une nouvelle demande
    do {
        int rank = get_owner(page_number);
        
        if (rank == DSM_NODE_ID)
            break;
        
        // Demande du numéro de page
        message_with_code(get_sckt_from_rank(proc_array, dsm_node_num, rank), &page_number, sizeof(int), OK_ASK_PAGE);
        
        // Attente de changement de propriétaire
        while (get_owner(page_number) == rank) {
            //~ sleep(1);
            // TODO : Trouver une solution ...
        }
    } while (rank != DSM_NODE_ID);
    
    // Et on peut tranquillement continuer notre exécution
}

/* traitant de signal adequat */
static void segv_handler(int sig, siginfo_t *info, void *context)
{
   /* A completer */
   /* adresse qui a provoque une erreur */
   void  *addr = info->si_addr;   
  /* Si ceci ne fonctionne pas, utiliser a la place :*/
  /*
   #ifdef __x86_64__
   void *addr = (void *)(context->uc_mcontext.gregs[REG_CR2]);
   #elif __i386__
   void *addr = (void *)(context->uc_mcontext.cr2);
   #else
   void  addr = info->si_addr;
   #endif
   */
   /*
   pour plus tard (question ++):
   dsm_access_t access  = (((ucontext_t *)context)->uc_mcontext.gregs[REG_ERR] & 2) ? WRITE_ACCESS : READ_ACCESS;   
  */   
   /* adresse de la page dont fait partie l'adresse qui a provoque la faute */
   void  *page_addr  = (void *)(((unsigned long) addr) & ~(PAGE_SIZE-1));
   int page_number = address2num(page_addr);

    if ((addr >= (void *)BASE_ADDR) && (addr < (void *)TOP_ADDR)) {
        dsm_handler(page_number);
        // TODO HERE
    }
    else {
    /* SIGSEGV normal : ne rien faire*/
    }
}


// Verbose de table_page
static void table_page_v() {
    int i;
    
    for (i = 0; i < PAGE_NUMBER; i++)
        fprintf(stdout, "(%i) %i:%i |", i, get_owner(i), get_status(i));
    
}

// Récupère les informations sur les processus frères envoyées par le
// le lanceur
void recup_info_proc(int sckt) {
    
    int j, i;
    // Enregistre les rangs des processus a supprimer :
    int rank_to_delete[dsm_node_num];
    
    // Nombre de processus ayant été effectivement lancé de dsmexec
    int proc_nb;
    
    // Variables temporaires pour l'enregistrement dans le tableau de
    // structures
    u_short proc_port, proc_rank;
    
    // Récupération du nombre de processus
    do_read(sckt, &proc_nb, sizeof(int));
    
    /* ================ Lecture des rangs de machines =============== */
    
    for(j= 0; j < proc_nb; j++) {
        // Rang
        do_read(sckt, &proc_rank, sizeof(u_short));
        
        // Port
        do_read(sckt, &proc_port, sizeof(u_short));
        
        if (VERBOSE) fprintf(stdout, "(%i) %i process. Rang %i : %i\n", j, proc_nb, proc_rank, proc_port);
        
        fill_proc_array(proc_array, dsm_node_num, proc_rank, proc_port);
    }
    
    // ----------------------- Suppression des processus non alloués ---
    
    // Allocation de -1 dans chaque cellule :
    memset(rank_to_delete, -1, dsm_node_num * sizeof(int));
    
    i = 0; // <= Index des struct. à supprimer
    
    // Recherche des structures à supprimer ( => qui n'ont pas de ports)
    for (j = 0; j < dsm_node_num; j++)
        if (proc_array[j].connect_info.port == 0) {
            rank_to_delete[i] = proc_array[j].connect_info.rank;
            i++;
        }
    
    // Suppression effective
    for (j = 0; j < dsm_node_num && rank_to_delete[j] != -1; j++)
        remove_from_rank(&proc_array, &dsm_node_num, rank_to_delete[j]);
}

// Explosions de connexions
void connexion_process(int sckt) {
    
    int i;
    // Structure de stockage temporaire pour les accept
    char ip_temporaire[INET_ADDRSTRLEN];
    // Numéro de descripteur de fichier temporaire
    int sckt_tmp = 666;
    // Variable temporaire de récupération du rang
    u_short rank;
    // Nbr de process de rang sup.
    u_short nb_process_sup = 0;
    
    // Nombre de processus de rang supérieur
    for (i = 0; i < dsm_node_num; i++)
        if (proc_array[i].connect_info.rank > DSM_NODE_ID)
            nb_process_sup++;
    
    do_listen(sckt, nb_process_sup);
    
    fflush(stdout);
    
    // Connexion aux autres processus
    for (i = 0; i < dsm_node_num; i++) {
        
        // Avant les connexions/acceptations étaient inversées. Mais il
        // semble plus logique de placer les acceptations dans les pro
        // -cessus lancés en premier.
        
        // Pour les rangs supérieurs, ce sont eux qui font la connexion
        if (proc_array[i].connect_info.rank > DSM_NODE_ID) {
            
            sckt_tmp = accept_and_rs_rank(sckt, &rank, DSM_NODE_ID);

            fill_proc_sckt(proc_array, dsm_node_num, rank, sckt_tmp);
            
            if (VERBOSE) fprintf(stdout, "Acceptation du process %i\n", rank);
            
        }
        
        // Pour les rangs inférieurs, c'est ce fichier qui établi la
        // connexion
        else {
            
            hostname_to_ip(proc_array[i].connect_info.machine_name, ip_temporaire);
            
            sckt_tmp = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            
            connect_and_sr_rank(
                sckt_tmp, // Socket
                *(get_addr_info(proc_array[i].connect_info.port, ip_temporaire)), // Adresse
                &rank, // Rang récupéré
                DSM_NODE_ID);
            
            fill_proc_sckt(proc_array, dsm_node_num, rank, sckt_tmp);
            
            if (VERBOSE) fprintf(stdout, "Connexion au process %i\n", rank);
            
        }
    }
    
}

/* Seules ces deux dernieres fonctions sont visibles et utilisables */
/* dans les programmes utilisateurs de la DSM                       */
char *dsm_init(int argc, char **argv) {
    struct sigaction act;
    int index;
    
    // Permettra d'attendre réception des pages dans dsm_comm_daemon
	//~ pthread_cond_init(&wait_page, NULL);
    // TODO <= ancien pthread init
   
    //                                               Depuis dsmwrap.c
    /* ============================================================== *\
                          Définition des variables
    \* ============================================================== */
    
    // Sockets
    int wrap_socket, wrap_socket_ecoute;
    
    // Adresse IP du dsmexec
    struct sockaddr_in *launcher_addr;
    char launcher_ip_addr[INET_ADDRSTRLEN];
    
    // Ports
    u_short launcher_port, wrap_port_ecoute;
    
    // Rank du processus
    DSM_NODE_ID = atoi(argv[3]);
    
    // Variable de boucle
    int i;
    
    /* ============================================================== *\
                                   Réseau
    \* ============================================================== */
    
    /* Creation de la socket d'ecoute pour les connexions avec les 
     * autres processus dsm */
    wrap_socket_ecoute = creer_socket(0, &wrap_port_ecoute, NULL);

    /* creation d'une socket pour se connecter au lanceur et envoyer/
     * recevoir les infos necessaires pour la phase dsm_init */
    wrap_socket = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    /* ======= Récupération de la struct sock_addr du lanceur ======= */
    hostname_to_ip(argv[1], launcher_ip_addr);

    launcher_port = atoi(argv[2]);
    
    if (VERBOSE) fprintf(stdout, "Adresse de %s : %s:%i\n", argv[1], launcher_ip_addr, launcher_port);
    
    launcher_addr = get_addr_info(launcher_port, launcher_ip_addr);

    /* ==================== Connexion au lanceur ==================== */
    do_connect(wrap_socket, *launcher_addr);

    /* Envoi du rang de processus au lanceur */
    handle_message(wrap_socket, &DSM_NODE_ID, sizeof(u_short));

    if (VERBOSE) printf("Port socket ecoute : %i \n", wrap_port_ecoute);

    /* Envoi du numero de port au lanceur. Le système choisit le port */ 
    handle_message(wrap_socket, &wrap_port_ecoute, sizeof(u_short));

    /* =============== Lecture du fichier de machines =============== */
    dsm_node_num = count_process_nb(argv[4]);
    proc_array = machine_names(argv[4], dsm_node_num);
    
    /* ============================================================== *\
          Récupération des infos de connexion aux autres processus
    \* ============================================================== */
    
    recup_info_proc(wrap_socket);

    // + Suppression de notre propre structure
    remove_from_rank(&proc_array, &dsm_node_num, DSM_NODE_ID);
    
    /* ============================================================== *\
                        Connexion aux autres processus
    \* ============================================================== */
    
    connexion_process(wrap_socket_ecoute);
    
    fflush(stdout);
    
    if (VERBOSE) for (i = 0; i <= argc; i++)
       fprintf(stdout, "Argv[%i] = %s\n", i, argv[i]);
   
   // Notre dsm_node_num correspond au nombre de processus à l'exception
   // de l'actuel
   
   /* Allocation des pages en tourniquet */
   for(index = 0; index < PAGE_NUMBER; index ++){    
     if ((index % (dsm_node_num+1)) == DSM_NODE_ID)
       dsm_alloc_page(index);         
     dsm_change_info( index, WRITE, index % (dsm_node_num+1));
   }
   
   /* mise en place du traitant de SIGSEGV */
   act.sa_flags = SA_SIGINFO; 
   act.sa_sigaction = segv_handler;
   sigaction(SIGSEGV, &act, NULL);
   
   /* creation du thread de communication */
   /* ce thread va attendre et traiter les requetes */
   /* des autres processus */
   pthread_create(&comm_daemon, NULL, dsm_comm_daemon, (void *) &dsm_node_num);
   
   /* Adresse de début de la zone de mémoire partagée */
   return ((char *)BASE_ADDR);
}

void dsm_ready_to_quit() {
	
	int i;
    
	for (i = 0; i < dsm_node_num; i++)
		message_with_code(proc_array[i].connect_info.socket, NULL, 0, OK_END);
	
}

void dsm_finalize( void ) {
    
    int i;
    
    // Indication aux autres process de l'attente de fermeture
    dsm_ready_to_quit();
    
    finalization = true;
    
    // Attente de self-terminaison du thread
    pthread_join(comm_daemon, NULL);
    
    /* fermer proprement les connexions avec les autres processus */

    // Placé avant la suppression des sockets (sinon remove_.. accède à 
    // dsm_node_num en même temps que le deamon)
    
    underlined("Fermeture des connexions.");
    
    // Fermeture des sockets + libération des structures
    for (i = dsm_node_num - 1; i >= 0 ; i--)
        remove_from_rank(&proc_array, &dsm_node_num, proc_array[i].connect_info.rank);

    fflush(stdout);

    return;
}
