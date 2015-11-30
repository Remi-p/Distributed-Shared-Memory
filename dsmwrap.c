#include "common_impl.h"

int main(int argc, char **argv)
{   
	fprintf(stdout, "Lancement de dsmwrap.c\n");
	
	/* processus intermediaire pour "nettoyer" */
	/* la liste des arguments qu'on va passer */
	/* a la commande a executer vraiment */
	int wrap_socket, l_wrap_socket;
	struct sockaddr_in *launcher_addr;
	char launcher_ip_addr[INET_ADDRSTRLEN];
	u_short launcher_port, wrap_port, b_wrap_port;
	int wrap_rank;
	pid_t pid;
	u_short proc_nb, proc_port, proc_rank;
	// Nombre de processus
	int num_procs;
	// Tableau des structures
	dsm_proc_t *proc_array = NULL; 

	/* Creation de la socket d'ecoute pour les */
	/* connexions avec les autres processus dsm */
	l_wrap_socket = creer_socket(0, &b_wrap_port, NULL);
	// TODO : Pas forcément besoin de récupérer l'adresse ip, normalement
	// 		 le serveur dsmexec la connaît.

	/* creation d'une socket pour se connecter au */
	/* au lanceur et envoyer/recevoir les infos */
	/* necessaires pour la phase dsm_init */ 
	wrap_socket = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//~ wrap_socket = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Récupération de la struct sock_addr du lanceur
	hostname_to_ip(argv[1], launcher_ip_addr);

	launcher_port = atoi(argv[2]);
	fprintf(stdout, "Adresse de %s : %s:%i\n", argv[1], launcher_ip_addr, launcher_port);
	launcher_addr = get_addr_info(launcher_port, launcher_ip_addr);

	// Connexion au lanceur
	do_connect(wrap_socket, *launcher_addr);

	/* Envoi du rang de processus au lanceur */
	wrap_rank = atoi(argv[3]);
	handle_message(wrap_socket, &wrap_rank, sizeof(u_short));

	/* Envoi du pid au lanceur */
	//~ pid = getpid();
	//~ handle_message(wrap_socket, &pid, sizeof(pid_t));

	printf("Port socket ecoute : %i \n", b_wrap_port);

	/* Envoi du numero de port au lanceur */
	/* le systeme choisit le port */ 
	handle_message(wrap_socket, &b_wrap_port, sizeof(u_short));


	/* =============== Lecture du fichier de machines =============== */
	num_procs = count_process_nb(argv[4]);
	proc_array = machine_names(argv[4], num_procs);
	
	// Récupération des noms de machines + structures
	// Récupération du nombre de processus
	do_read(wrap_socket, &proc_nb, sizeof(u_short), NULL);
	int j;
	for(j= 0; j < proc_nb; j++) {
		// Rang
		do_read(wrap_socket, &proc_rank, sizeof(u_short), NULL);
		
		// Port
		do_read(wrap_socket, &proc_port, sizeof(u_short), NULL);
		
		fill_proc_array(proc_array, num_procs, proc_rank, proc_port);
	}

	/* pour qu'il le propage à tous les autres */
	/* processus dsm */

	/* on execute la bonne commande */

	short int i= 0;

	for (i = 0; i <= argc; i++) {
	   if (VERBOSE) fprintf(stdout, "Argv[%i] = %s\n", i, argv[i]);
	}

	// TOASK : Le premier 'bonjour à tous' n'apparaît pas ?
	fprintf(stderr, "Bonjour à tous !\n");

	return 0;
}
