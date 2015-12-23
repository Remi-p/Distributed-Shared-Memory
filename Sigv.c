#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h> 
#include <poll.h>
#include <time.h>
#include <ucontext.h>

static void sigactiontest(int sig, siginfo_t* info, void* cntxt) {
	
	//~ ucontext_t *context = (ucontext_t *) cntxt;
	
	fprintf(stdout, "Bonne réception du signal %i\n", sig);
}

int main(int argc, char *argv[]) {
	
	struct sigaction bonjour; //, sizeof(struct sigaction));
	
	bonjour.sa_sigaction = &sigactiontest;
	bonjour.sa_flags = SA_SIGINFO;
	
	sigaction(SIGSEGV, &bonjour, NULL);
	
	// Segv :
	int* entier = NULL;
	*entier = 7;
	
	exit(EXIT_SUCCESS);
	
}
