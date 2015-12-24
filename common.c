#include "common_impl.h"

#include <stdarg.h>

// Affiche une ligne jusqu'à l'EOF ou une nouvelle ligne
bool disp_line(FILE* out, int in) {
	
	// TOASK : Optimisé (par fprintf) ou à optimiser à la main ?
	char c;
	int ret;
	bool first = true;
	
	while ( (ret = read(in, &c, sizeof(char))) != 0 ) {
		// Face à une erreur autre qu'une interruption de signal
		if (ret == -1) {
			if (errno != EINTR)
				break;
		}
		
		// Pas d'erreurs du tout :
		else {
			if (c == '\0' || c == '\n')
				break;
			
			if (first) {
				first = false;
				fprintf(out, "\t");
			}
			
			fprintf(out, "%c", c);
		}
			
	}
	
	if (ret < 0)
		error("Erreur de lecture de tube (disp_line) ");
	
	else if (ret == 0)
		return false;
	
	fprintf(out, "\n");
	fflush(out);
	
	return true;
	
}

// Liste les informations de connexion liées à un tableau de struct :
void display_connect_info(dsm_proc_t *process, int num_process) {
	
	int i;
	
	for (i = 0; i < num_process; i++)
		fprintf(stdout, "Process. de rang %i :\n\tsocket : %i\n\tport : %i\n\tnom : %s\n",
			process[i].connect_info.rank,
			process[i].connect_info.socket,
			process[i].connect_info.port,
			process[i].connect_info.machine_name);
	
}

// Verifie que les noms de machine sont en alphanumeriques
// Check is a string is alphanumeric
int check_machine_name(char * name) {
	int i;
	for(i = 0; i < strlen(name); i++) {
		if(isalnum(name[i]) == false) {
			return false;
		}
	}
	return true;
}

// Affiche un texte souligné
void underlined(char *text, ...) {
	
	fprintf(stdout, "%s", ANSI_STYLE_UNDERLINED);
	
	va_list arglist;
	va_start( arglist, text );
	vfprintf( stdout, text, arglist );
	va_end( arglist );
   
	fprintf(stdout, "%s", ANSI_RESET);
}

// Enlève un élément du tableau de processus
void remove_from_rank(dsm_proc_t** process, int* nb_process, int rank) {
	
	int i;
	int proc_qty = *nb_process;
	
	for (i = 0; i < proc_qty; i++) {
		
		// La cellule à supprimer à été localisée
		if((*process)[i].connect_info.rank == rank) {
	
			// Fermeture des descripteurs de fichier
			close((*process)[i].stderr);
			close((*process)[i].stdout);
			
			// Fermeture de la socket
			if ((*process)[i].connect_info.socket != 0)
				close((*process)[i].connect_info.socket);
			
			// Si ce n'est pas le dernier élément, on déplace la mémoire
			if (i != proc_qty -1)
				memmove((*process) + i,
						(*process) + i + 1,
						(sizeof(struct dsm_proc) * ( proc_qty - 1 - i)) );
			
			// Réallocation de la mémoire avec la taille nécessaire
			*process = realloc(*process, (proc_qty - 1) * sizeof(struct dsm_proc));
			
			if (VERBOSE) fprintf(stdout, "Délétion du processus de rang %i\n", rank);
			
			(*nb_process)--;
			return;
		}
		
	}
	
	fprintf(stderr, "Rank %i not found\n", rank);
	error("");
}

/* compte le nombre de processus a lancer */
int count_process_nb(char * machine_file) {
	
	int c;
	unsigned int process_nb = 0;
	FILE * fichier;
	
	if( (fichier = fopen(machine_file, "r")) == NULL)
		return -1;
		
	while( (c = fgetc(fichier)) != EOF ){
		if(c == '\n')
			++process_nb;
	}
	
	return process_nb;
}

/* retourne un tableau de struct dsm_proc */
/* de taille du nombre de processus */
/* contenant le nom de la machine + le rang */
dsm_proc_t* machine_names(char * name_file, int process_nb) {
	
	FILE * fichier;
	dsm_proc_t* proc_array = malloc(process_nb * sizeof(struct dsm_proc));
	int i = 0;
	char * machine = (char *) malloc(NAME_MAX * sizeof(char));
	
	if( (fichier = fopen(name_file, "r")) == NULL)
		perror("fopen");
		
	while(fgets(machine, NAME_MAX, fichier) != NULL) {
		
		proc_array[i].connect_info.machine_name = (char *) malloc(strlen(machine) - 1);
		
		// Ceci remplace le saut de ligne par une fin de ligne, et stop
		// strcpy
		machine[strlen(machine) - 1] = '\0';
		
		// On enlève le retour chariot en même temps que l'on copie la chaîne
		if(check_machine_name(machine) == false)
			error("check_machine_name");
			
		strcpy(proc_array[i].connect_info.machine_name, machine);
		
		// On enregistre le rang, car des machines pourront être fermées
		// et changer ainsi l'ordre naturel du tableau
		proc_array[i].connect_info.rank = i;
		
		i++;
	}
	
	return proc_array;
	
}

// Segmentation fault
void gdb_stop() {	
	// http://stackoverflow.com/questions/18986351/what-is-the-simplest-standard-conform-way-to-produce-a-segfault-in-c
	const char *s = NULL;
	printf( "%c\n", s[0] );
}

// Fonction d'erreur
void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Enregistre le port au processus de rang rank
int fill_proc_array(dsm_proc_t *proc_array, int num_procs, u_short rank, u_short port)
{
	int i;
	for(i=0; i < num_procs; i++) {
		if(proc_array[i].connect_info.rank == rank) {
			proc_array[i].connect_info.port = port;
			return true;
		}
	}
	return false;
}
