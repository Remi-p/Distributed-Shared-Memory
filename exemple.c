#include "dsm.h"

int main(int argc, char **argv) {
	
	char *pointer; 
	char *current;
	int value;

	underlined("Lancement de exemple.c");
    fflush(stdout);

	pointer = dsm_init(argc,argv);
	current = pointer;
    
	fprintf(stdout, "[%i] Coucou, mon adresse de base est : %p\n", DSM_NODE_ID, pointer);

	if (DSM_NODE_ID == 0) {
		sleep(1);
		
        current += 4*sizeof(int);
        value = *((int *)current);
        
        fprintf(stdout, "[%i] valeur de l'entier : %i\n", DSM_NODE_ID, value);
	} 
	else if (DSM_NODE_ID == 2) {
        // Accès au même endroit que le processus de rang précédent :
        current += 4*sizeof(int);
        *((int *)current) = 42;
        value = *((int *)current);
        
        fprintf(stdout, "[%i] valeur de l'entier : %i\n", DSM_NODE_ID, value);
	}
	
	dsm_finalize();
	return 1;
}
