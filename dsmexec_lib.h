void usage(void);

// Affiche un texte souligné
void underlined(char *text, ...);
// Affiche un text en gras
void bold(char *text, ...);

void sigchld_handler(int sig);

// Lecture dans un tube, tant qu'il y a des données
void lecture_tube(int fd);

// Création des fils + lancement des ssh
void lancement_processus_fils(int num_procs,dsm_proc_t *proc_array,
char* ip, u_short port_num, int argc, char *argv[], volatile int *num_procs_creat);

/* Retourne un tableau de struct dsm_proc  de taille du nombre de
 * processus contenant le nom de la machine + le rang */
dsm_proc_t* machine_names(char * name_file, int process_nb);

/* compte le nombre de processus à lancer */
int count_process_nb(char * machine_file);

// Enlève un élément du tableau de processus
void remove_from_rank(dsm_proc_t** process, int* nb_process, int rank);

// Acceptation des connexions et enregistrement des informations
void acceptation_connexions(int* num_procs, int listen_socket,dsm_proc_t **proc_array);

// Affichage des données reçues sur les tubes
void affichage_tubes(int *num_procs, dsm_proc_t **proc_array);

// Verifie que les noms de machine sont en alphanumeriques
int check_machine_name(char * name);
