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

// Acceptation des connexions et enregistrement des informations
void acceptation_connexions(int* num_procs, int listen_socket, dsm_proc_t **proc_array);

// Affichage des données reçues sur les tubes
void affichage_tubes(int *num_procs, dsm_proc_t **proc_array);

// Envoi des ports à toutes les machines
void envoi_port(dsm_proc_t **proc_array, int* num_procs);
