/* ==================== Projet Système et Réseau ==================== *\
 * Gourdel Thibaut                                          . \|/ /,  *
 * Perrot Remi            =================                 \\`''/_-- *
 * PR204                  |    dsmwrap    |          Bordeaux INP --- *
 * Décembre 2015          | lanceur local |          ENSEIRB  ,..\\`  *
 *                        =================          MATMECA / | \\`  *
\* ================================================================== */
#include "common_impl.h"

// Récupère les informations sur les processus frère envoyé par le
// le lanceur
void recup_info_proc(int sckt, dsm_proc_t **proc_array, int *num_procs) {
    
    int j, i;
    // Enregistre les rangs des processus a supprimer :
    int rank_to_delete[*num_procs];
    
    // Nombre de processus ayant été effectivement lancé de dsmexec
    int proc_nb;
    
    // Variables temporaires pour l'enregistrement dans le tableau de
    // structures
    u_short proc_port, proc_rank;
    
    // Récupération du nombre de processus
    do_read(sckt, &proc_nb, sizeof(int), NULL);
    
    /* ================ Lecture des rangs de machines =============== */
    
    for(j= 0; j < proc_nb; j++) {
        // Rang
        do_read(sckt, &proc_rank, sizeof(u_short), NULL);
        
        // Port
        do_read(sckt, &proc_port, sizeof(u_short), NULL);
        
        fprintf(stdout, "(%i) %i process. Rang %i : %i\n", j, proc_nb, proc_rank, proc_port);
        
        fill_proc_array(*proc_array, *num_procs, proc_rank, proc_port);
    }
    
    // ----------------------- Suppression des processus non alloués ---
    
    // Allocation de -1 dans chaque cellule :
    memset(rank_to_delete, -1, *num_procs * sizeof(int));
    
    i = 0; // <= Index des struct. à supprimer
    
    // Recherche des structures à supprimer ( => qui n'ont pas de ports)
    for (j = 0; j < *num_procs; j++)
        if ((*proc_array)[j].connect_info.port == 0) {
            rank_to_delete[i] = (*proc_array)[j].connect_info.rank;
            i++;
        }
    
    // Suppression effective
    for (j = 0; j < *num_procs && rank_to_delete[j] != -1; j++)
        remove_from_rank(proc_array, num_procs, rank_to_delete[j]);
}

/* processus intermediaire pour "nettoyer" la liste des arguments qu'on 
 * va passer a la commande a executer vraiment */
int main(int argc, char **argv)
{   
    underlined("Lancement de dsmwrap.c, rang %s\n", argv[3]);
    
    /* ============================================================== *\
                          Définition des variables
    \* ============================================================== */
    
    // Sockets
    int wrap_socket, wrap_socket_ecoute;
    
    // Adresse IP du dsmexec
    struct sockaddr_in *launcher_addr;
    char launcher_ip_addr[INET_ADDRSTRLEN];
    
    // Structure de stockage temporaire pour les accept
    char ip_temporaire[INET_ADDRSTRLEN];
    
    // Ports
    u_short launcher_port, wrap_port_ecoute;
    
    // Rank du processus
    u_short self_rank = atoi(argv[3]);
    
    // Nombre de processus estimé depuis le fichier de machines
    int num_procs;
    
    // Tableau des structures des processus dsmwrap
    dsm_proc_t *proc_array = NULL;
    
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
    self_rank = atoi(argv[3]);
    handle_message(wrap_socket, &self_rank, sizeof(u_short));

    if (VERBOSE) printf("Port socket ecoute : %i \n", wrap_port_ecoute);

    /* Envoi du numero de port au lanceur. Le systeme choisit le port */ 
    handle_message(wrap_socket, &wrap_port_ecoute, sizeof(u_short));

    /* =============== Lecture du fichier de machines =============== */
    num_procs = count_process_nb(argv[4]);
    proc_array = machine_names(argv[4], num_procs);
    
    /* ============================================================== *\
          Récupération des infos de connexion aux autres processus
    \* ============================================================== */
    
    recup_info_proc(wrap_socket, &proc_array, &num_procs);

    // + Suppression de notre propre structure
    remove_from_rank(&proc_array, &num_procs, self_rank);
    
    /* ============================================================== *\
                        Connexion aux autres processus
    \* ============================================================== */
    
    do_listen(wrap_socket_ecoute, num_procs);
    
    // Connexion aux autres processus
    for (i = 0; i < num_procs; i++) {
        
        // Pour les rangs supérieurs, ce sont eux qui font la connexion
        if (proc_array[i].connect_info.rank > self_rank) {
            struct sockaddr *test = calloc(0, sizeof(struct sockaddr));
            proc_array[i].connect_info.socket = do_accept(wrap_socket_ecoute, test);
            fprintf(stdout, "Acceptation du process %i\n", i);
        }
        
        // Pour les rangs inférieurs, c'est ce fichier qui établi la
        // connexion
        else {
            hostname_to_ip(proc_array[i].connect_info.machine_name, ip_temporaire);
            proc_array[i].connect_info.socket = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            do_connect(proc_array[i].connect_info.socket,
                       *(get_addr_info(proc_array[i].connect_info.port,
                                       ip_temporaire)) );
            fprintf(stdout, "Connexion au process %i\n", i);
        }
    }
    
    /* pour qu'il le propage à tous les autres */
    /* processus dsm */

    /* on execute la bonne commande */

    for (i = 0; i <= argc; i++) {
       if (VERBOSE) fprintf(stdout, "Argv[%i] = %s\n", i, argv[i]);
    }

    // TOASK : Le premier 'bonjour à tous' n'apparaît pas ?
    fprintf(stderr, "Bonjour à tous !\n");
    
    // Fermeture des sockets

    return 0;
}
