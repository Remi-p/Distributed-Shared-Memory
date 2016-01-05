#include "common_impl.h"
#include "common_net.h"

#include "dsmexec_lib.h"

#include <stdarg.h>

// Affiche une mini aide lors d'une erreur de saisie de commande
void usage(void) {
    fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
    fflush(stdout);
    exit(EXIT_FAILURE);
}

// Passage d'un descripteur en mode non-bloquant
void non_bloquant(int fd) {
    int flags;
    
    flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

// Affiche un texte en gras
void bold(char *text, ...) {
    
    fprintf(stdout, "%s", ANSI_STYLE_BOLD);
    
    va_list arglist;
    va_start( arglist, text );
    vfprintf( stdout, text, arglist );
    va_end( arglist );
   
    fprintf(stdout, "%s", ANSI_RESET);
}

// Gestion de la terminaison des fils
void sigchld_handler(int sig) {
    /* on traite les fils qui se terminent */
    /* pour eviter les zombies */

    int ret;

    // Gestion du risque de recevoir plusieurs signaux
    do {
        ret = waitpid(0, NULL, WNOHANG);
    } while(ret > 0);
}

// Création des fils + lancement des ssh
void lancement_processus_fils(int num_procs, dsm_proc_t *proc_array,
char* ip, u_short port_num, int argc, char *argv[], volatile int *num_procs_creat) {
    
    // Définition des variables =======================================
    pid_t pid;
    int i, j;
    char exec_dsmwrap[BUFFER_CHEMIN];
    char path_machines[BUFFER_CHEMIN];
    char str[BUFFER_CHEMIN];
    // Répertoire courant :
    char *wd_ptr = NULL;
    wd_ptr = getcwd(str, BUFFER_CHEMIN);
    
    // Variables temporaires pour les tubes pour stdout et stderr
    int fd_stdout[2];   // fd_*[0] : extremité en lecture
    int fd_stderr[2];   // fd_*[1] : extremité en écriture
    
    // Il y aura des descripteurs de fichier hérités dans le fils, qu'il
    // faudra fermer (à partir du second fils créé)
    int fd_to_close[2];
    
    // Nom de la machine serveur
    char * hostname = malloc(sizeof(char) * NAME_MAX);
    gethostname(hostname, sizeof(char) * NAME_MAX);
    
    // Port du serveur
    char * port = malloc(sizeof(char) * 5); // La taille maximale d'un port est 5 chiffres
    sprintf(port, "%i", port_num); // Port du serveur
    
    // Chemin vers le programme distant
    sprintf(exec_dsmwrap, "%s/%s", wd_ptr, argv[2]);
    
    // Chemin vers machine_file
    sprintf(path_machines, "%s/%s", wd_ptr, argv[1]);
    
    // ------------- Nombres d'arguments pour le ssh
    // 7 = Nombre d'arguments entré en "dur"
    // argc = Nombre d'arguments passés lors de l'exécution de dsmexec
    // -3 = On enlève [le nom de fichier & le nom du fichier machine] de argv
    //        & le nom du programme lancé sur les machines distantes
    // 1 = Dernier élement, = NULL
    u_short newargc = 7 + argc - 3 + 1;
    
    // Tableau de chaîne de caractères, pour stocker les arguments
    char **newargv = malloc(sizeof(char *) * newargc);
    // (Ne sera jamais free, puisque l'execvp quitte le programme)
    
    /* ============================================================== *\
                 Creation du tableau d'arguments pour le ssh 
                (les parties ne changeant pas en fct du fils)
    \* ============================================================== */
    newargv[0] = "ssh";
    // 1 : machine name
    newargv[2] = exec_dsmwrap;
    newargv[3] = hostname; // Hostname du serveur (fichier courant)
    newargv[4] = port; // Port du serveur
    // 5 : rang du processus
    newargv[6] = path_machines; // fichier de la machine
    
    // Pour les autres arguments, on complète avec ceux donnés lors de l'appel
    for (j = 3; j < argc; j++) {
        newargv[j + newargc - argc] = malloc(sizeof(char) * strlen(argv[j]));
        strcpy(newargv[j + newargc - argc], argv[j]);
    }
    
    // Dernier de la liste d'arguments
    newargv[newargc - 1] = NULL;
    // ----------------------------------------------------------------
    
    for(i = 0; i < num_procs ; i++) {

        /* creation du tube pour rediriger stdout */
        if (pipe(fd_stdout) == -1)
            error("Erreur de création du tube de redirection stdout ");

        /* creation du tube pour rediriger stderr */
        if (pipe(fd_stderr) == -1)
            error("Erreur de création du tube de redirection sterr ");

        if (VERBOSE) fprintf(stdout, "Création du fils n°%i\n", i);
        pid = fork();
        
        if(pid == -1)
            error("Erreur lors du fork ");

        if (pid == 0) { /* fils */    
            
            // Problème à gérer ; le premier processus n'a pas à
            // fermer de descripteurs
            if (*num_procs_creat > 0) {
                // Inutile pour le fils :
                close(fd_to_close[0]);
                close(fd_to_close[1]);
            }

            /* redirection stdout */
            close(fd_stdout[0]); // Fermeture de l'extrémité inutilisée

            close(STDOUT_FILENO); // On remplace stdout par fd[1]
            dup(fd_stdout[1]);
            close(fd_stdout[1]);

            /* redirection stderr */      
            close(fd_stderr[0]);

            close(STDERR_FILENO);
            dup(fd_stderr[1]);
            close(fd_stderr[1]);

            /* ====================================================== *\
                     Creation du tableau d'arguments pour le ssh
                       (parties dynamiques en fonction du fils)
            \* ====================================================== */
            newargv[1] = proc_array[i].connect_info.machine_name;
            newargv[5] = malloc(sizeof(char) * 3);
                sprintf(newargv[5], "%i", i); // Rang du processus (distant)

            // =================== Lancement du ssh ================= //
            if (execvp("ssh", newargv) == -1)
                error("Erreur lors de l'exécution ssh ");

        } else if(pid > 0) { /* pere */
            
            /* fermeture des extremites des tubes non utiles */
            // Fermeture de l'extremité d'écriture, puisqu'on souhaite lire.
            close(fd_stderr[1]);
            close(fd_stdout[1]);
            
            // Passage en mode non-bloquant (voir explication dans la
            // fonction disp_line() )
            non_bloquant(fd_stderr[0]);
            non_bloquant(fd_stdout[0]);
            
            // Enregistrement des extrémités utiles pour le reste du
            // programme :
            proc_array[i].stderr = fd_stderr[0];
            proc_array[i].stdout = fd_stdout[0];
            
            // À supprimer dans le fils
            fd_to_close[0] = fd_stderr[0];
            fd_to_close[1] = fd_stdout[0];

            (*num_procs_creat)++;
        }
    }
}

// Acceptation des connexions et enregistrement des informations
void acceptation_connexions(int* num_procs, int listen_socket, dsm_proc_t **proc_array ) {
    
    // Définition des variables =======================================
    
    // Stockage temporaire de l'adresse des clients
    struct sockaddr* adr_tmp;
    // Retour de do_accept
    int accept_sckt;
    // Rang et port du programme distant
    u_short rank_machine;
    u_short wrap_port;
    // Variables de boucles
    int i, j, k;
    // Stockage des descripteurs de fichiers pour le select
    fd_set readfds;
    // Timeout pour le select (permet de ne pas prendre en compte les
    // ordinateurs où le programme ne s'est pas lancé)
    struct timeval timeout;
        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;
    // Résultat du select
    int res;
    
    // ----------------------------------------------------------------
       
    // Acceptation des connexions par les processus distants
    for (i = 0; i < *num_procs ; i++) {
        
        adr_tmp = malloc(sizeof(struct sockaddr));
        
        // Réinitialisation du set pour select
        FD_ZERO(&readfds);
        FD_SET(listen_socket, &readfds);
        
        // Si on a une interruption par un signal on recommence
        while ((res = select(listen_socket + 1, &readfds, NULL, NULL, &timeout)) == -1)
            if ( errno != EINTR )
                error("Erreur lors du select ");
        
        if(res != 0) {
            
            accept_sckt = do_accept(listen_socket, adr_tmp);
        
            // On récupère le rang de la machine dans le tableau de la
            // structure, plutôt que son nom
            do_read(accept_sckt, &rank_machine, sizeof(u_short));
            
            /* On recupere le numero de port de la socket d'ecoute des 
             * processus distants */
            do_read(accept_sckt, &wrap_port, sizeof(u_short));
            
            fprintf(stdout, "Rang deviné : %i / Port deviné : %i\n", rank_machine, wrap_port);
            
            // On enregistre le fd de la socket dans la structure
            // associée au rang renvoyé
            for(j=0; j < *num_procs; j++)
                if((*proc_array)[j].connect_info.rank == rank_machine) { 
                    (*proc_array)[j].connect_info.socket = accept_sckt;
                    (*proc_array)[j].connect_info.port = wrap_port;
                }
        }
        
        free(adr_tmp);
    }
    // Si des accepts ne se sont pas fait -> machine eteinte par exple
    // On les supprime du tableau de structure
    for(k=0; k < *num_procs; k++)
        if((*proc_array)[k].connect_info.socket == 0) 
            remove_from_rank(proc_array, num_procs, (*proc_array)[k].connect_info.rank) ;

}

// Affichage des données reçues sur les tubes
void affichage_tubes(int *num_procs, dsm_proc_t **proc_array) {
    
    int i;
    
    // Necessaire pour l'argument du poll
    int nb_tubes = 0;
    
    // On va enregistrer une fois pour toutes les descripteurs ici -----
    struct pollfd *fds = calloc(sizeof(struct pollfd), sizeof(struct pollfd) * (*num_procs * 2));
    nb_tubes = 0;

    if (VERBOSE) bold("\n== Ajout des tubes du processus au poll\n");

    // Placement des descripteurs de fichier dans le tableau de
    // structure de la variable fds.
    for (i = 0; i < *num_procs ; i++) {
        
        // stderr
        fds[2*i+1].fd = (*proc_array)[i].stderr;
        fds[2*i+1].events = POLLIN | POLLHUP;
        
        // stdout
        fds[2*i].fd = (*proc_array)[i].stdout;
        fds[2*i].events = POLLIN | POLLHUP;
        
        if (VERBOSE) fprintf(stdout, "(i=%i) : %i (err) & %i (out)\n",
                            i, fds[2*i+1].fd, fds[2*i].fd);
        
        nb_tubes += 2;
    }
    // -----------------------------------------------------------------

    while(*num_procs > 0) {
        
        // Poll ====================================================
        
        // On fait une boucle while pour effectuer le poll tant que
        // l'on a l'erreur EINTR (interrompu par un signal)
        while ( poll( fds, nb_tubes, 0) == -1 )
            if ( errno != EINTR )
                error("Erreur lors du select ");
        
        //~ if (VERBOSE) bold("\n== Boucle for d'affichage, %i process\n", *num_procs);
        
        // Le poll à réussi :
        for (i = *num_procs - 1; i >= 0 ; i--) {
            
            if (fds[2*i].revents & POLLIN) {
                
                // stdout
                fprintf(stdout, "[%s > proc %i > stdout]\n",
                    (*proc_array)[i].connect_info.machine_name,
                    (*proc_array)[i].connect_info.rank);
                    
                while (disp_line(stdout, (*proc_array)[i].stdout) != false);
            }
            
            if (fds[2*i+1].revents & POLLIN) {
                
                // stderr
                fprintf(stdout, "[%s%s > proc %i > stderr%s]\n", ANSI_COLOR_RED,
                    (*proc_array)[i].connect_info.machine_name,
                    (*proc_array)[i].connect_info.rank, ANSI_RESET);
                
                while (disp_line(stdout, (*proc_array)[i].stderr) != false);
            }
            
            // On ne ferme le tube qu'après avoir affiché les
            // informations
            if (fds[2*i].revents & POLLHUP || fds[2*i+1].revents & POLLHUP) {
                
                remove_from_rank(proc_array, num_procs, (*proc_array)[i].connect_info.rank);
                
                // Suppressions des deux descripteurs d'un coup de 'fds'
                remove_any(fds, nb_tubes/2, 2 * sizeof(struct pollfd), i);
                
                // Decrémentation du nombre de tubes
                nb_tubes -= 2;
                
                // Remarque : on a deplacé la mémoire pour proc_array et
                //            pour fds, donc les positions relatives sont
                //            toujours les mêmes pour les deux tableaux
            }
        }

    };
    
    free(fds);
}

// Envoi des ports à toutes les machines
void envoi_port(dsm_proc_t **proc_array, int* num_procs) {
    
    int num = *num_procs;
    int sckt;
    int i;
    
    int tab_size = ( sizeof(u_short) * 2 ) * num;
    
    // Création du tableau rang/port à envoyer -------------------------
    u_short* tableau = malloc( tab_size );
    // On parcourt le tableau de case en case. Une case étant composée
    // de deux u_short : rang + port
    for (i = 0; i < num; i++) {
        // Rang
        tableau[2*i]   = (*proc_array)[i].connect_info.rank;
        tableau[2*i+1] = (*proc_array)[i].connect_info.port;
    }
    // -----------------------------------------------------------------
    
    for (i = 0; i < num; i++) {
        
        sckt = (*proc_array)[i].connect_info.socket;
        
        // Envoi du nombre de processus
        handle_message(sckt, &num, sizeof(int));
        
        // Envoi de la liste complète
        handle_message(sckt, tableau, tab_size);
        
    }
    
    free(tableau);
    
}
