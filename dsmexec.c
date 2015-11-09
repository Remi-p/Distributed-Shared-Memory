#include "common_impl.h"

/* variables globales */

/* un tableau gerant les infos d'identification */
/* des processus dsm */
dsm_proc_t *proc_array = NULL; 

/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0;

void usage(void)
{
  fprintf(stdout,"Usage : dsmexec machine_file executable arg1 arg2 ...\n");
  fflush(stdout);
  exit(EXIT_FAILURE);
}

void sigchld_handler(int sig)
{
	/* on traite les fils qui se terminent */
	/* pour eviter les zombies */

	int ret;
   
	// Gestion du risque de recevoir plusieurs signaux
	do {
		ret = waitpid(0, NULL, WNOHANG);
	} while(ret > 0);
}

int main(int argc, char *argv[])
{
  if (argc < 3){
    usage();
  } else {       
     pid_t pid;
     int num_procs = 0;
     int i;
	struct sigaction sigact;
	// Tubes pour stdout et stderr
	int fd_stdout[2]; 	// fd_stdout[0] : extremité en lecture
						// fd_stdout[1] : extremité en écriture
	int fd_stderr[2];
	int result;
	
     /* Mise en place d'un traitant pour recuperer les fils zombies*/    
	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = &sigchld_handler;
	sigaction(SIGCHLD, &sigact, NULL);
     
     /* lecture du fichier de machines */
     /* 1- on recupere le nombre de processus a lancer */
     num_procs = count_process_nb("machine_file");
     
     /* 2- on recupere les noms des machines : le nom de */
     /* la machine est un des elements d'identification */
     proc_array = machine_names("machine_file", num_procs);
     
     /* creation de la socket d'ecoute */
     int listen_socket = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
     
     /* + ecoute effective */
     while(1) {
		FD_ZERO(&readset);
		FD_SET(listen_socket, &readset);
		result = select(sock_client + 1, &readset, NULL, NULL, NULL);
		
		if (result > 0) {
			if (FD_ISSET(listen_socket, &readset)) {
			/*
			 * S'il y a une activit sur la socket
			 * 
			 * /
			} 
		}
	 }
     
     /* creation des fils */
     for(i = 0; i < num_procs ; i++) {
	
	/* creation du tube pour rediriger stdout */
	pipe(fd_stdout);
	
	/* creation du tube pour rediriger stderr */
	pipe(fd_stderr);
	
	pid = fork();
	if(pid == -1) ERROR_EXIT("fork");
	
	if (pid == 0) { /* fils */	
	   
	   /* redirection stdout */
		close(fd_stdout[0]); // Fermeture de l'extrémité inutilisée
		
		close(STDOUT_FILENO);		 // On remplace stdout par fd[1]
		dup(fd_stdout[1]);
		close(fd_stdout[1]);
	   
	   /* redirection stderr */	  
		close(fd_stderr[0]);
		
		close(STDERR_FILENO);
		dup(fd_stderr[1]);
		close(fd_stderr[1]);
	   
	   /* Creation du tableau d'arguments pour le ssh */ 
	   
	   /* jump to new prog : */
	   /* execvp("ssh",newargv); */

	} else  if(pid > 0) { /* pere */		      
	   /* fermeture des extremites des tubes non utiles */
	   
		// Fermeture de l'extremité d'écriture, puisqu'on souhaite lire.
		close(fd_stderr[1]);
		close(fd_stdout[1]);
		// TODO : Fermeture des autres extrémités non utilisées
		
	   num_procs_creat++;	      
	}
     }
     
   
     for(i = 0; i < num_procs ; i++){
	
	/* on accepte les connexions des processus dsm */
	
	/*  On recupere le nom de la machine distante */
	/* 1- d'abord la taille de la chaine */
	/* 2- puis la chaine elle-meme */
	
	/* On recupere le pid du processus distant  */
	
	/* On recupere le numero de port de la socket */
	/* d'ecoute des processus distants */
     }
     
     /* envoi du nombre de processus aux processus dsm*/
     
     /* envoi des rangs aux processus dsm */
     
     /* envoi des infos de connexion aux processus */
     
     /* gestion des E/S : on recupere les caracteres */
     /* sur les tubes de redirection de stdout/stderr */     
     /* while(1)
         {
            je recupere les infos sur les tubes de redirection
            jusqu'à ce qu'ils soient inactifs (ie fermes par les
            processus dsm ecrivains de l'autre cote ...)
       
         };
      */
     
     /* on attend les processus fils */
     
     /* on ferme les descripteurs proprement */
     
     /* on ferme la socket d'ecoute */
  }   
   exit(EXIT_SUCCESS);  
}

