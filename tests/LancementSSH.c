#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


// Fonction d'erreur
void error(const char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
	
	char **newargv = malloc(sizeof(char *) * 7);
	
	newargv[0] = "ssh";
	newargv[1] = "soremade";
	newargv[2] = "/net/malt/t/rperrot/Cours/PR204/Distributed-Shared-Memory/bin/dsmwrap";
	newargv[3] = "IP_DU_SERVEUR"; // TODO
	newargv[4] = "PORT_DU_SERVEUR"; // TODO
	newargv[5] = "RANG_DU_PROCESS_DISTANT"; // TODO
	newargv[6] = NULL;
	
	/* jump to new prog : */
	if (execvp("ssh", newargv) == -1)
		error("Erreur lors de l'exécution ssh ");
	
	return 0;
}


