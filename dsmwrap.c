#include "common_impl.h"

int main(int argc, char **argv)
{   
    fprintf(stdout, "Lancement de dsmwrap.c\n");
    
    /* processus intermediaire pour "nettoyer" */
    /* la liste des arguments qu'on va passer */
    /* a la commande a executer vraiment */
    int wrap_socket, l_wrap_socket;
    struct sockaddr_in *launcher_addr;
    char launcher_ip_addr[INET_ADDRSTRLEN];
    char ip_temporaire[INET_ADDRSTRLEN];
    u_short launcher_port, wrap_port, b_wrap_port;
    u_short self_rank = atoi(argv[3]);
    int wrap_rank;
    pid_t pid;
    int proc_nb; u_short proc_port, proc_rank;
    // Nombre de processus
    int num_procs;
    // Tableau des structures
    dsm_proc_t *proc_array = NULL;
    // Variables temporaires de boucles
    int j, i;

    /* Creation de la socket d'ecoute pour les */
    /* connexions avec les autres processus dsm */
    l_wrap_socket = creer_socket(0, &b_wrap_port, NULL);
    // TODO : Pas forcément besoin de récupérer l'adresse ip, normalement
    //          le serveur dsmexec la connaît.

    /* creation d'une socket pour se connecter */
    /* au lanceur et envoyer/recevoir les infos */
    /* necessaires pour la phase dsm_init */ 
    wrap_socket = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    //~ wrap_socket = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Récupération de la struct sock_addr du lanceur
    hostname_to_ip(argv[1], launcher_ip_addr);

    launcher_port = atoi(argv[2]);
    fprintf(stdout, "Adresse de %s : %s:%i\n", argv[1], launcher_ip_addr, launcher_port);
    launcher_addr = get_addr_info(launcher_port, launcher_ip_addr);

    // Connexion au lanceur
    do_connect(wrap_socket, *launcher_addr);

    /* Envoi du rang de processus au lanceur */
    wrap_rank = atoi(argv[3]);
    handle_message(wrap_socket, &wrap_rank, sizeof(u_short));

    /* Envoi du pid au lanceur */
    //~ pid = getpid();
    //~ handle_message(wrap_socket, &pid, sizeof(pid_t));

    printf("Port socket ecoute : %i \n", b_wrap_port);

    /* Envoi du numero de port au lanceur */
    /* le systeme choisit le port */ 
    handle_message(wrap_socket, &b_wrap_port, sizeof(u_short));


    /* =============== Lecture du fichier de machines =============== */
    num_procs = count_process_nb(argv[4]);
    proc_array = machine_names(argv[4], num_procs);
    
    // Récupération des noms de machines + structures
    // Récupération du nombre de processus
    do_read(wrap_socket, &proc_nb, sizeof(int), NULL);
    
    /* ================ Lecture des rangs de machines =============== */
    for(j= 0; j < proc_nb; j++) {
        // Rang
        do_read(wrap_socket, &proc_rank, sizeof(u_short), NULL);
        
        // Port
        do_read(wrap_socket, &proc_port, sizeof(u_short), NULL);
        
        fprintf(stdout, "(%i) %i process. Rang %i : %i\n", j, proc_nb, proc_rank, proc_port);
        
        fill_proc_array(proc_array, num_procs, proc_rank, proc_port);
    }
    
    /* ================ Connexions aux autres process =============== */
    // Suppression des processus non alloué
    int rank_to_delete[num_procs];
    i= 0;
    memset(rank_to_delete, -1, num_procs * sizeof(int));
    
    for (j = 0; j < num_procs; j++)
        if (proc_array[j].connect_info.port == 0) {
            rank_to_delete[i] = proc_array[j].connect_info.port;
            i++;
        }
    
    for (j = 0; j < num_procs && rank_to_delete[j] != -1; j++)
        remove_from_rank(&proc_array, &num_procs, rank_to_delete[j]);
    
    // + Suppression de notre propre structure
    remove_from_rank(&proc_array, &num_procs, self_rank);
    
    do_listen(l_wrap_socket, num_procs);
    
    // Connexion aux autres processus
    for (i = 0; i < num_procs; i++) {
        // Pour les rangs inférieurs, ce sont eux qui font la connexion
        if (proc_array[i].connect_info.rank < self_rank) {
            struct sockaddr *test = calloc(0, sizeof(struct sockaddr));
            proc_array[i].connect_info.socket = do_accept(l_wrap_socket, test);
            fprintf(stdout, "Connexion au process %i\n", i);
        }
        
        // Pour les rangs supérieurs, c'est ce fichier qui établi la
        // connexion
        else {
            hostname_to_ip(proc_array[i].connect_info.machine_name, ip_temporaire);
            proc_array[i].connect_info.socket = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            do_connect(proc_array[i].connect_info.socket, 
                       *(get_addr_info(proc_array[i].connect_info.port,
                                       ip_temporaire)) );
            fprintf(stdout, "Acception au process %i\n", i);
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
