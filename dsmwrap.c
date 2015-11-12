#include "common_impl.h"

int main(int argc, char **argv)
{   
   /* processus intermediaire pour "nettoyer" */
   /* la liste des arguments qu'on va passer */
   /* a la commande a executer vraiment */
   int wrap_socket;
   int conn;
   struct sockaddr_in *launcher_addr, *wrap_addr;
   char * launcher_ip_addr;
   int launcher_port, wrap_port;
   int wrap_rank;
   
   /* creation d'une socket pour se connecter au */
   /* au lanceur et envoyer/recevoir les infos */
   /* necessaires pour la phase dsm_init */ 
   wrap_socket = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   
   // Récupération de la struct sock_addr du lanceur
   launcher_ip_addr = argv[1];
   launcher_port = atoi(argv[2]);
   launcher_addr = get_addr_info(&launcher_port, launcher_ip_addr);
   
   // Connexion au lanceur
   conn = do_connect(wrap_socket, *launcher_addr);
   
   /* Envoi du nom de machine au lanceur */
   // VERIF : envoie du rang plutot
   wrap_rank = atoi(argv[4]);
   handle_message(wrap_socket, &wrap_rank, sizeof(u_short));

   /* Envoi du pid au lanceur */

   /* Creation de la socket d'ecoute pour les */
   /* connexions avec les autres processus dsm */

   /* Envoi du numero de port au lanceur */
   // le systeme choisit le port
   wrap_addr = get_addr_info(0, "localhost");
   wrap_port = wrap_addr->sin_port;
   handle_message(wrap_socket, &wrap_port, sizeof(u_short));
   // VERIF : pas sur que ca marche d'envoyer un u_short
   
   /* pour qu'il le propage à tous les autres */
   /* processus dsm */

   /* on execute la bonne commande */
   
   short int i= 0;
   
   for (i = 0; i <= argc; i++) {
	   fprintf(stdout, "Argv[%i] = %s\n", i, argv[i]);
   }
   
   fprintf(stdout, "Bonjour à tous !\n");
   
   return 0;
}
