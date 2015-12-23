#include "dsm.h"

int dsm_node_num; /* nombre de processus dsm */
int DSM_NODE_ID;  /* rang (= numero) du processus */ 

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
    
    if( numpage > PAGE_NUMBER || numpage < 0 )
        error("Le nombre de page trouvé est incorrect");
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

static void *dsm_comm_daemon( void *arg) {  
    
	// Ici on stockera toutes les sockets des autres processus
	fd_set socks;
    int plus_grde_sckt;
    int retour_select;
    int i;
    int sckt_tmp;
    
    while(dsm_node_num > 0) {
        FD_ZERO(&socks);
		
        for (i = 0; i < dsm_node_num; i++) {
            sckt_tmp = PROC_ARRAY[i].connect_info.socket;
            FD_SET(sckt_tmp, &socks);
            
            if (plus_grde_sckt < sckt_tmp)
                plus_grde_sckt = sckt_tmp;
            // Il faut changer dynamiquement plus_grde_sckt puisque des
            // descripteurs peuvent être fermés
        }
		
        fprintf(stdout, "BEFORE SELECT\n");
        retour_select = select( plus_grde_sckt + 1,
								&socks, NULL, NULL, NULL);
        fprintf(stdout, "AFTER SELECT : %i\n", retour_select);
        
        // Rmq : il se peut que le daemon soit arrêté avant de sortir du
        //       select.
		if (retour_select < 0)
			error("Erreur lors du select");
            
        // Si on arrive là, c'est qu'une connexion a été fermée ou qu'un
        // processus veut accéder à notre page/mettre à jour le proprio.
        
        for (i = 0; i < dsm_node_num; i++) {
            sckt_tmp = PROC_ARRAY[i].connect_info.socket;
            
            if (FD_ISSET(sckt_tmp, &socks)) {
                fprintf(stdout, "Du mvt sur %i !\n", sckt_tmp);
                fflush(stdout);
            }
        }
    }
    
   while(1)
     {
    /* a modifier */
    printf("[%i] Waiting for incoming reqs \n", DSM_NODE_ID);
    sleep(2);
     }
   return;
}

// TODO : En fonction du rang (from rank), envoi, réception, etc.

static int dsm_send(int dest,void *buf,size_t size)
{
   /* a completer */
}

static int dsm_recv(int from,void *buf,size_t size)
{
   /* a completer */
}

static void dsm_handler( int page_number )
{  
   /* A modifier */
   
   // Récupération du numéro de page
   
   
   printf("[%i] FAULTY  ACCESS sur la page de l'utilisateur %i !!! \n",DSM_NODE_ID, get_owner(page_number));
   abort();
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
        
        fill_proc_array(PROC_ARRAY, dsm_node_num, proc_rank, proc_port);
    }
    
    // ----------------------- Suppression des processus non alloués ---
    
    // Allocation de -1 dans chaque cellule :
    memset(rank_to_delete, -1, dsm_node_num * sizeof(int));
    
    i = 0; // <= Index des struct. à supprimer
    
    // Recherche des structures à supprimer ( => qui n'ont pas de ports)
    for (j = 0; j < dsm_node_num; j++)
        if (PROC_ARRAY[j].connect_info.port == 0) {
            rank_to_delete[i] = PROC_ARRAY[j].connect_info.rank;
            i++;
        }
    
    // Suppression effective
    for (j = 0; j < dsm_node_num && rank_to_delete[j] != -1; j++)
        remove_from_rank(&PROC_ARRAY, &dsm_node_num, rank_to_delete[j]);
}

// Explosions de connexions
void connexion_process(int sckt) {
    
    int i;
    // Structure de stockage temporaire pour les accept
    char ip_temporaire[INET_ADDRSTRLEN];
    // Variable temporaire de récupération du rang
    u_short rank;
    // Nbr de process de rang inf.
    u_short nb_process_inf = 0;
    
    // Nombre de processus de rang inférieur
    for (i = 0; i < dsm_node_num && PROC_ARRAY[i].connect_info.rank < DSM_NODE_ID; i++)
        nb_process_inf++;
    
    do_listen(sckt, nb_process_inf);
    
    fflush(stdout);
    
    // Connexion aux autres processus
    for (i = 0; i < dsm_node_num; i++) {
        
        // Pour les rangs inférieurs, ce sont eux qui font la connexion
        if (PROC_ARRAY[i].connect_info.rank < DSM_NODE_ID) {
            
            PROC_ARRAY[i].connect_info.socket
                = accept_and_rs_rank(sckt, &rank, DSM_NODE_ID);
            
            fprintf(stdout, "Acceptation du process %i\n", rank);
            
        }
        
        // Pour les rangs supérieurs, c'est ce fichier qui établi la
        // connexion
        else {
            hostname_to_ip(PROC_ARRAY[i].connect_info.machine_name, ip_temporaire);
            
            PROC_ARRAY[i].connect_info.socket = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            
            connect_and_sr_rank(
                PROC_ARRAY[i].connect_info.socket, // Socket
                *(get_addr_info(PROC_ARRAY[i].connect_info.port, ip_temporaire)), // Adresse
                &rank, // Rang récupéré
                DSM_NODE_ID);
            
            fprintf(stdout, "Connexion au process %i\n", rank );
            
        }
    }
    
}

/* Seules ces deux dernieres fonctions sont visibles et utilisables */
/* dans les programmes utilisateurs de la DSM                       */
char *dsm_init(int argc, char **argv) {
    struct sigaction act;
    int index;
   
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
    PROC_ARRAY = machine_names(argv[4], dsm_node_num);
    
    /* ============================================================== *\
          Récupération des infos de connexion aux autres processus
    \* ============================================================== */
    
    recup_info_proc(wrap_socket);

    // + Suppression de notre propre structure
    remove_from_rank(&PROC_ARRAY, &dsm_node_num, DSM_NODE_ID);
    
    /* ============================================================== *\
                        Connexion aux autres processus
    \* ============================================================== */
    
    connexion_process(wrap_socket_ecoute);
    
    for (i = 0; i <= argc; i++)
       if (VERBOSE) fprintf(stdout, "Argv[%i] = %s\n", i, argv[i]);
    
   
   /* reception du nombre de processus dsm envoye */
   /* par le lanceur de programmes (dsm_node_num)*/
   
   /* reception de mon numero de processus dsm envoye */
   /* par le lanceur de programmes (DSM_NODE_ID)*/
   
   /* reception des informations de connexion des autres */
   /* processus envoyees par le lanceur : */
   /* nom de machine, numero de port, etc. */
   
   /* initialisation des connexions */ 
   /* avec les autres processus : connect/accept */
   
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
   pthread_create(&comm_daemon, NULL, dsm_comm_daemon, NULL);
   
   /* Adresse de début de la zone de mémoire partagée */
   return ((char *)BASE_ADDR);
}

void dsm_finalize( void ) {
    
    int i;
    
    /* fermer proprement les connexions avec les autres processus */

    // Fermeture des sockets + libération des structures
    for (i = dsm_node_num - 1; i >= 0 ; i--)
        remove_from_rank(&PROC_ARRAY, &dsm_node_num, PROC_ARRAY[i].connect_info.rank);

    fflush(stdout);

    /* terminer correctement le thread de communication */
    /* pour le moment, on peut faire : */
    pthread_cancel(comm_daemon);

    return;
}
