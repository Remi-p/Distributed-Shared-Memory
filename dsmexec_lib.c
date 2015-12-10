#include "common_impl.h"
#include "common_net.h"

#include "dsmexec_lib.h"

#include <stdarg.h>

void usage(void) {
	fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
	fflush(stdout);
	exit(EXIT_FAILURE);
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

void sigchld_handler(int sig) {
	/* on traite les fils qui se terminent */
	/* pour eviter les zombies */

	int ret;

	// Gestion du risque de recevoir plusieurs signaux
	do {
		ret = waitpid(0, NULL, WNOHANG);
	} while(ret > 0);
}

// Lecture dans un tube, tant qu'il y a des données
void lecture_tube(int fd) {
	
	// TODO : Afficher les infos d'un processus en tabulé
	
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

void lancement_processus_fils(int num_procs, dsm_proc_t *proc_array,
char* ip, u_short port_num, int argc, char *argv[], volatile int *num_procs_creat) {
	
	// Définition des variables =======================================
	pid_t pid;
	int i, j;
	char exec_dsmwrap[1024];
	char path_machines[1024];
	char str[1024];
	char *wd_ptr = NULL;
	wd_ptr = getcwd(str,1024);
	
	// Variables temporaires pour les tubes pour stdout et stderr
	int fd_stdout[2]; 	// fd_stdout[0] : extremité en lecture
						// fd_stdout[1] : extremité en écriture
	int fd_stderr[2];
	
	// Il y aura des descripteurs de fichier hérités dans le fils, qu'il
	// faudra fermer
	int fd_to_close[2];
	
	// Nom de la machine serveur
	char * hostname = malloc(sizeof(char) * NAME_MAX);
	gethostname(hostname, sizeof(char) * NAME_MAX);
	
	// Port du serveur
	char * port = malloc(sizeof(char) * 5); // La taille maximale d'un port est 5 chiffres
	sprintf(port, "%i", port_num); // Port du serveur
	
	// Chemin vers dsmwrap
	sprintf(exec_dsmwrap, "%s/bin/dsmwrap", wd_ptr);
	
	// Chemin vers machine_file
	sprintf(path_machines, "%s/%s", wd_ptr, argv[1]);
	
	// ------------- Nombres d'arguments pour le ssh
	// 7 = Nombre d'arguments entré en "dur"
	// argc = Nombre d'arguments passés lors de l'exécution de dsmexec
	// -2 = On enlève [le nom de fichier & le nom du fichier machine] de argv
	// 1 = Dernier élement, = NULL
	u_short newargc = 7 + argc - 2 + 1;
	// ----------------------------------------------------------------
	
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

			//~ // Inutile pour le fils :
			close(fd_to_close[0]);
			close(fd_to_close[1]);

			if (VERBOSE) printf("Création du fils n°%i\n", i);

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

			/* ====================================================== *\
					 Creation du tableau d'arguments pour le ssh 
			\* ====================================================== */
			char **newargv = malloc(sizeof(char *) * newargc);

			newargv[0] = "ssh";
			newargv[1] = proc_array[i].connect_info.machine_name;
			newargv[2] = exec_dsmwrap;
			newargv[3] = hostname; // Hostname du serveur (fichier courant)
			newargv[4] = port; // Port du serveur
			newargv[5] = malloc(sizeof(char) * 3);
				sprintf(newargv[5], "%i", i); // Rang du processus (distant)
			newargv[6] = path_machines; // fichier de la machine
			
			// Pour les autres arguments, on complète avec ceux donnés lors de l'appel
			for (j = 2; j < argc; j++) {
				newargv[j + 7 - 2] = malloc(sizeof(char) * strlen(argv[j]));
				strcpy(newargv[j + 5], argv[j]);
			}
			// Dernier de la liste d'arguments
			newargv[newargc - 1] = NULL;
			/* ------------------------------------------------------ *\
			\* ------------------------------------------------------ */

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
			
			// À supprimer dans le fils
			fd_to_close[0] = fd_stderr[0];
			fd_to_close[1] = fd_stdout[0];

			(*num_procs_creat)++;
		}
	}
}

// Acceptation des connexions et enregistrement des informations
void acceptation_connexions(int* num_procs, int listen_socket, dsm_proc_t **proc_array ) {
	
	struct sockaddr* adr_tmp;
	int accept_sckt;
	u_short rank_machine;
	u_short wrap_port;
	// pid_t wrap_pid;
	int i, j, k;
	fd_set readfds;
	struct timeval timeout;
		timeout.tv_sec = TIMEOUT;
		timeout.tv_usec = 0;
	int res;
        
	for (i = 0; i < *num_procs ; i++) {
        
		/* on accepte les connexions des processus dsm */
		adr_tmp = malloc(sizeof(struct sockaddr));
		
		FD_ZERO(&readfds);
		FD_SET(listen_socket, &readfds); 
		
		// fprintf(stdout, "Select pour accept (n°%i)\n", i);
		
		while ((res = select(listen_socket + 1, &readfds, NULL, NULL, &timeout)) == -1)
			if ( errno != EINTR )
				error("Erreur lors du select ");
		
		if(res != 0) {
			
			accept_sckt = do_accept(listen_socket, adr_tmp);
		
			// On récupère le rang de la machine dans le tableau de la
			// structure, plutôt que son nom
			do_read(accept_sckt, &rank_machine, sizeof(u_short), NULL);
			
			/* On recupere le pid du processus distant  */
			// do_read(accept_sckt, &wrap_pid, sizeof(pid_t), NULL);
			
			//fprintf(stdout, "pid deviné : %i\n", wrap_pid);

			/* On recupere le numero de port de la socket */
			/* d'ecoute des processus distants */
			do_read(accept_sckt, &wrap_port, sizeof(u_short), NULL);
			
			fprintf(stdout, "Rang deviné : %i / Port deviné : %i\n", rank_machine, wrap_port);
			
			// On enregistre le fd de la socket
			// dans la structure associée au rang renvoyé
			for(j=0; j < *num_procs; j++) {
				if((*proc_array)[j].connect_info.rank == rank_machine) { 
					(*proc_array)[j].connect_info.socket = accept_sckt;
					(*proc_array)[j].connect_info.port = wrap_port;
				}
			}
		}
		
		free(adr_tmp);
		// ERROR potentielle
	}
	// Si des accepts ne ce sont pas fait -> machine eteinte par ex
	// On les supprime du tableau de structure
	for(k=0; k < *num_procs; k++) {
		if((*proc_array)[k].connect_info.socket == 0) 
			remove_from_rank(proc_array, num_procs, (*proc_array)[k].connect_info.rank) ;
	}
	
}

// Affichage des données reçues sur les tubes
void affichage_tubes(int *num_procs, dsm_proc_t **proc_array) {
	
	int i;
	
	// Necessaire pour l'argument du poll
	int nb_tubes;
	// Buffer de réception sur les tubes
	char buffer[BUFFER_MAX];
	memset(buffer, 0, BUFFER_MAX * sizeof(char));
	
	// Ici on stockera tous les descripteurs de fichier pour les tubes
	struct pollfd *fds = calloc(sizeof(struct pollfd), sizeof(struct pollfd) * (*num_procs * 2));
	// On utilise un malloc pour pouvoir utiliser le realloc

	while(*num_procs > 0) {
		
		fds = realloc(fds, sizeof(struct pollfd) * (*num_procs * 2));
		nb_tubes = 0;
	
		if (VERBOSE) bold("\n== Ajout des tubes du processus au poll\n");
	
		for (i = 0; i < *num_procs ; i++) {
			// stdout
			fds[2*i].fd = (*proc_array)[i].stdout;
			fds[2*i].events = POLLIN | POLLHUP;
			 
			// stderr
			fds[2*i+1].fd = (*proc_array)[i].stderr;
			fds[2*i+1].events = POLLIN | POLLHUP;
			
			if (VERBOSE) fprintf(stdout, "(i=%i) : %i & %i\n",
								i, (*proc_array)[i].stderr, (*proc_array)[i].stdout);
			
			nb_tubes += 2;
		}
		
		// Poll ====================================================
		
		// On fait une boucle while pour effectuer le poll tant que
		// l'on a l'erreur EINTR (interrompu par un signal)
		while ( poll( fds, nb_tubes, 0) == -1 )
			if ( errno != EINTR )
				error("Erreur lors du select ");
		
		if (VERBOSE) bold("\n== Boucle for d'affichage, %i process\n", *num_procs);
		
		// Le poll à réussi :
		for (i = 0; i < *num_procs ; i++) {
	
			if ((fds[2*i].events & POLLIN) == POLLIN) {
				
				fprintf(stdout, "[%s%s > proc %i > stderr%s]\n", ANSI_COLOR_RED,
					(*proc_array)[i].connect_info.machine_name,
					(*proc_array)[i].connect_info.rank, ANSI_RESET);
				
				lecture_tube((*proc_array)[i].stderr);
				
				fprintf(stdout, "\n");
				fflush(stdout);
			}
			
			if ((fds[2*i+1].events & POLLIN) == POLLIN) {
				
				fprintf(stdout, "[%s > proc %i > stdout]\n",
					(*proc_array)[i].connect_info.machine_name,
					(*proc_array)[i].connect_info.rank);
				
				lecture_tube((*proc_array)[i].stdout);
				
				fprintf(stdout, "\n");
				fflush(stdout);
			}
			
			// On ne ferme le tube qu'après avoir affiché les
			// informations
			if ((fds[2*i].events & POLLHUP) == POLLHUP || (fds[2*i+1].events & POLLHUP) == POLLHUP) {
				remove_from_rank(proc_array, num_procs, (*proc_array)[i].connect_info.rank);
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
	
	u_short u_short_size = sizeof(u_short);
	int tab_size = (u_short_size*2) * num;
	
	// TODO : Tester avec u_short
	
	// Création du tableau à envoyer
	u_short* tableau = malloc( tab_size );
	// On parcourt le tableau de case en case. Une case étant composée
	// de deux u_short : rang + port
	for (i = 0; i < num; i++) {
		// Rang
		tableau[2*i] = (*proc_array)[i].connect_info.rank;
		tableau[2*i+1] = (*proc_array)[i].connect_info.port;
	}
	
	for (i = 0; i < *num_procs; i++) {
		
		sckt = (*proc_array)[i].connect_info.socket;
		
		// Envoi du nombre de processus
		handle_message(sckt, &num, sizeof(int));
		
		// Envoi de la liste complète
		handle_message(sckt, tableau, tab_size);
		
	}
	
	free(tableau);
	
}
