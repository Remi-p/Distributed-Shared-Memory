#include "common_impl.h"

/* variables globales */

/* un tableau gerant les infos d'identification */
/* des processus dsm */
dsm_proc_t *proc_array = NULL; 

/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0;

void usage(void) {
	fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
	fflush(stdout);
	exit(EXIT_FAILURE);
}

void sigchld_handler(int sig) {
	/* on traite les fils qui se terminent */
	/* pour eviter les zombies */

	int ret;

	// Gestion du risque de recevoir plusieurs signaux
	do {
		ret = waitpid(0, NULL, WNOHANG);
	} while(ret > 0);
}

void lecture_tube(int fd) {
	
	char *buffer = malloc(sizeof(char) * BUFFER_MAX);
	
	off_t off = 0;
	
	do {
		
		memset(buffer, 0, sizeof(char) * BUFFER_MAX);
		
		off = read(fd, buffer, BUFFER_MAX);
		
		if (off == -1 && errno != EINTR )
			error("Erreur de lecture d'un tube ");
		
		// Si on a été interrompu par le système, on recommence !
		if (errno == EINTR)
			off = 1;
		else
			fprintf(stdout, "%s", buffer);
	} while (off > 0);
	
	free(buffer);
}

int main(int argc, char *argv[]) {
	
	fprintf(stdout, "Lancement de dsmexec.c\n");
	
	if (argc < 3){
		usage();
	} else {
		
		// Définition des variables ====================================
		
		enum code code_ret; // Code de retour
		pid_t pid;
		int num_procs = 0;
		int i, j;
		struct sigaction sigact;
		// Variables temporaires pour les tubes pour stdout et stderr
		int fd_stdout[2]; 	// fd_stdout[0] : extremité en lecture
							// fd_stdout[1] : extremité en écriture
		int fd_stderr[2];
		struct sockaddr* adr_tmp;

		/* Mise en place d'un traitant pour recuperer les fils zombies*/    
		memset(&sigact, 0, sizeof(struct sigaction));
		sigact.sa_handler = &sigchld_handler;
		sigaction(SIGCHLD, &sigact, NULL);
		
		// Variables liées au select -----------------------------------
		// Ici on stockera tous les descripteurs de fichier pour les
		// tubes
		fd_set readset;
		// Necessaire pour le select
		int plus_grd_tube;
		// Buffer de réception des tubes
		char buffer[BUFFER_MAX];
		memset(buffer, 0, BUFFER_MAX * sizeof(char));

		/* lecture du fichier de machines */
		/* 1- on recupere le nombre de processus a lancer */
		num_procs = count_process_nb(argv[1]);
		// TODO : Vérification nom machine (alphanumérique)

		/* 2- on recupere les noms des machines : le nom de */
		/* la machine est un des elements d'identification */
		proc_array = machine_names(argv[1], num_procs);

		/* creation de la socket d'ecoute */
		int listen_socket = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		// Initialisation de la structure sockaddr_in pour l'adresse du server
		struct sockaddr_in* serv_add = get_addr_info( DEFAULT_PORT, NULL);
		
		// TODO : Clean
		// Pour l'adresse IP à donné aux processus distants, on récupère
		// les adresses des interfaces de la machine
		struct ifaddrs *ifaddr;
		if (getifaddrs(&ifaddr) == -1)
			error("Erreur de récupération de l'adresse IP ");
		struct ifaddrs *present = ifaddr;
		char *ip = NULL;
		do {
			// Pour une raison inconnue, il y a des versions sans le 
			// netmask. Pour trouver la bonne adresse on regarde donc
			// qu'il soit définie.
			if (strcmp(present->ifa_name, "em1") == 0
			&& present->ifa_netmask != NULL) {
				ip = inet_ntoa( ((struct sockaddr_in* ) present->ifa_addr)->sin_addr );
				break;
			}
			present = present->ifa_next;
		} while (present != NULL);
		freeifaddrs(ifaddr);
		if (ip == NULL)
			error("IP de la machine non trouvée ");
		
		// On bind la socket sur le port TCP spécifié auparavant
		do_bind(listen_socket, serv_add);
		
		// On enregistre le port après le bind, car il a été
		// potentiellement changé
		u_short port = ntohs(serv_add->sin_port);
		
		/* + ecoute effective */
		do_listen(listen_socket, num_procs);

		if (VERBOSE) printf("== Boucle de création des fils\n");
				
		/* creation des fils */
		for(i = 0; i < num_procs ; i++) {

			/* creation du tube pour rediriger stdout */
			if (pipe(fd_stdout) == -1)
				error("Erreur de création du tube de redirection stdout ");

			/* creation du tube pour rediriger stderr */
			if (pipe(fd_stderr) == -1)
				error("Erreur de création du tube de redirection sterr ");

			pid = fork();
			
			if(pid == -1) ERROR_EXIT("fork");

			if (pid == 0) { /* fils */	

				if (VERBOSE) printf("Fils n°%i\n", i);

				/* redirection stdout */
				close(fd_stdout[0]); // Fermeture de l'extrémité inutilisée

				close(STDOUT_FILENO);// On remplace stdout par fd[1]
				dup(fd_stdout[1]);
				close(fd_stdout[1]);

				/* redirection stderr */	  
				close(fd_stderr[0]);

				close(STDERR_FILENO);
				dup(fd_stderr[1]);
				close(fd_stderr[1]);

				/* Creation du tableau d'arguments pour le ssh */ 
				// if (VERBOSE) printf("Création du tableau d'arguments pour le ssh\n");

				// 6 = Nombre d'arguments ci-après
				// argc = Nombre d'arguments passés lors de l'exécution de dsmexec
				// -2 = On enlève [le nom de fichier & le nom du fichier machine] de argv
				// 1 = Dernier élement, = NULL
				u_short newargc = 6 + argc - 2 + 1;
				char **newargv = malloc(sizeof(char *) * newargc);

				newargv[0] = "ssh";
				newargv[1] = proc_array[i].connect_info.machine_name;
				newargv[2] = "/net/malt/t/rperrot/Cours/PR204/Distributed-Shared-Memory/bin/dsmwrap";
				newargv[3] = ip; // IP du serveur (fichier courant)
				newargv[4] = malloc(sizeof(char) * 5); // La taille maximale d'un port est 5 chiffres
					sprintf(newargv[4], "%i", port); // Port du serveur
				newargv[5] = malloc(sizeof(char) * 3);
					sprintf(newargv[5], "%i", i); // Rang du processus (distant)
				
				// Pour les autres arguments, on complète avec ceux donnés lors de l'appel
				for (j = 2; j < argc; j++) {
					newargv[j + 6 - 2] = malloc(sizeof(char) * strlen(argv[j]));
					strcpy(newargv[j + 4], argv[j]);
				}
				// Dernier de la liste d'arguments
				newargv[newargc - 1] = NULL;
				
				// if (VERBOSE) printf("Nom machine de rang %i => [%s]\n", i, newargv[1]);

				/* jump to new prog : */
				if (execvp("ssh", newargv) == -1)
					error("Erreur lors de l'exécution ssh ");

			} else  if(pid > 0) { /* pere */
				
				/* fermeture des extremites des tubes non utiles */
				// Fermeture de l'extremité d'écriture, puisqu'on souhaite lire.
				close(fd_stderr[1]);
				close(fd_stdout[1]);
				
				// Enregistrement des extrémités utiles pour le reste du
				// programme :
				proc_array[i].stderr = fd_stderr[0];
				proc_array[i].stdout = fd_stdout[0];

				num_procs_creat++;
			}
		}

		// TODO : À placer dans l'initialisation
		int rank_machine;
		
		for (i = 0; i < num_procs ; i++) {
		
			/* on accepte les connexions des processus dsm */
			adr_tmp = malloc(sizeof(struct sockaddr));
			int accept_sckt = do_accept(listen_socket, adr_tmp);
			
			// TOASK : Pas de récupération du nom de machine ?
			/*  On recupere le nom de la machine distante */
			/* 1- d'abord la taille de la chaine */
			/* 2- puis la chaine elle-meme */
			
			// On récupère plutot le rang de la machine dans le tableau
			// de la structure
			do_read(accept_sckt, &rank_machine, sizeof(int), NULL);
			
			// TODO : Le rang n'est jamais trouvé, on reste à chaque
			// fois à 0.
			fprintf(stdout, "Rang deviné : %i\n", rank_machine);

			/* On recupere le pid du processus distant  */
			
			/* On recupere le numero de port de la socket */
			/* d'ecoute des processus distants */
		}

		/* envoi du nombre de processus aux processus dsm*/

		/* envoi des rangs aux processus dsm */

		/* envoi des infos de connexion aux processus */

		/* gestion des E/S : on recupere les caracteres */
		/* sur les tubes de redirection de stdout/stderr */     
		while(1) {
			
			// R.A.Z ===================================================
			FD_ZERO(&readset);
				
			for (i = 0; i < num_procs ; i++) {
			
				FD_SET(proc_array[i].stderr, &readset);
				FD_SET(proc_array[i].stdout, &readset);
				
				fprintf(stdout, "%i : %i & %i\n", i, proc_array[i].stderr, proc_array[i].stdout);
				
			}
			
			plus_grd_tube = proc_array[i].stderr;
			// ---------------------------------------------------------
			
			// Select ==================================================
			if ( select( plus_grd_tube + 1, &readset, NULL, NULL, NULL) == -1
			&& errno != EINTR)
				error("Erreur lors du select ");
			
			// TOASK : Est-ce qu'on fait vraiment ça ?
			// Si on a l'erreur EINTR, c'est qu'on a été interrompu par
			// un signal. On continue quand même et on observe les sets
			else
				for (i = 0; i < num_procs ; i++) {
			
				if (FD_ISSET(proc_array[i].stderr, &readset)) {
					
					fprintf(stdout, "[%s%s > proc %i > stderr%s]\n", ANSI_COLOR_RED, proc_array[i].connect_info.machine_name, i, ANSI_COLOR_RESET);
					
					lecture_tube(proc_array[i].stderr);
					
					fprintf(stdout, "\n");
					fflush(stdout);
				}
					
				if (FD_ISSET(proc_array[i].stdout, &readset)) {
					
					fprintf(stdout, "[%s > proc %i > stdout]\n", proc_array[i].connect_info.machine_name, i);
					
					lecture_tube(proc_array[i].stdout);
					
					fprintf(stdout, "\n");
					fflush(stdout);
				}
				
			}
			
			
			/* je recupere les infos sur les tubes de redirection
			jusqu'à ce qu'ils soient inactifs (ie fermes par les
			processus dsm ecrivains de l'autre cote ...) */
			
			

		};

		/* on attend les processus fils */

		/* on ferme les descripteurs proprement */

		/* on ferme la socket d'ecoute */
	}
	
	exit(EXIT_SUCCESS);  
}

