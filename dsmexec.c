#include "common_impl.h"

#include "dsmexec_lib.h"

/* variables globales */

/* un tableau gerant les infos d'identification des processus dsm */
dsm_proc_t *proc_array = NULL; 

/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0;

int main(int argc, char *argv[]) {
    
    underlined("\nLancement de dsmexec.c\n\n");
    
    if (argc < 3){
        usage();
    } else {
        /* ========================================================== *\
                        Définition des variables
        \* ========================================================== */
        
        // Nombre de processus (depuis le nombre de machine)
        int num_procs;
        // enum code code_ret; // Code de retour (à gérer)
        
        // Numéro de descripteur de fichier pour la socket d'écoute
        int listen_socket;
        
        // Récupération dynamique n° de port + IP
        u_short port_num;
        char *ip = NULL;
        
        /* Mise en place d'un traitant pour recuperer les fils zombies*/
        struct sigaction sigact; // Pour la gestion du signal  
        memset(&sigact, 0, sizeof(struct sigaction));
        sigact.sa_handler = &sigchld_handler;
        sigaction(SIGCHLD, &sigact, NULL);

        /* ========================================================== *\
                        Lecture du fichier de machines
        \* ========================================================== */
        
        /* 1- on recupere le nombre de processus a lancer */
        num_procs = count_process_nb(argv[1]);

        /* 2- on recupere les noms des machines : le nom de */
        /* la machine est un des elements d'identification */
        proc_array = machine_names(argv[1], num_procs);

        
        /* ==================== Socket d'écoute ===================== */
        listen_socket = creer_socket(0, &port_num, &ip);
        
        /* Écoute effective */
        do_listen(listen_socket, num_procs);

        /* ========================================================== *\
                              Création des fils
        \* ========================================================== */
        if (VERBOSE) bold("= Boucle de création des fils\n");
        
        lancement_processus_fils(num_procs, proc_array, ip, port_num, argc, argv, &num_procs_creat);
        
        /* ========================================================== *\
                          Acceptation des connexions
        \* ========================================================== */
        if (VERBOSE) bold("\n= Boucle d'acceptation de connexion\n");
        
        acceptation_connexions(&num_procs, listen_socket, &proc_array);

        /* envoi du nombre de processus aux processus dsm*/
		
        /* envoi des rangs aux processus dsm */

        /* envoi des infos de connexion aux processus */

        /* gestion des E/S : on recupere les caracteres */
        /* sur les tubes de redirection de stdout/stderr */     
        
        /* ========================================================== *\
                     Affichage des données reçues sur les tubes
        \* ========================================================== */
        if (VERBOSE) bold("\n= Boucle de lecture des tubes\n");
        
        affichage_tubes(&num_procs, &proc_array);
        
        /* ======================= Extinction ======================= */
        fprintf(stdout, "Extinction du programme.\n");

        /* on attend les processus fils */
        // <= Gérer avec sigchld_handler()

        /* on ferme les descripteurs proprement */
        // <= Gérer dans affichage_tubes()

        /* on ferme la socket d'ecoute */
        close(listen_socket);
    }
    
    exit(EXIT_SUCCESS);  
}

