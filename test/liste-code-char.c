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

int main(int argc, char **argv) {
	
	fprintf(stdout, "\\n : %i\n", '\n');
	fprintf(stdout, "\\b : %i\n", '\b');
	fprintf(stdout, "\\r : %i\n", '\r');
	fprintf(stdout, "\\v : %i\n", '\v');
	fprintf(stdout, "\\t : %i\n", '\t');
}
