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

// http://stackoverflow.com/questions/10348591/how-to-list-all-file-descriptors-of-a-process-in-c-without-calling-lsof-or-pars
int count_fds(void)
{
    int maxfd = getdtablesize();
    int openfds;
    int fd;
    int i;

    openfds = 0;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
            return maxfd;
    close(fd);

    for (i = 0; i < maxfd; i++) {
            if (dup2(i, fd) != -1) {
                    openfds++;
                    fprintf(stdout, "[%i]", i);
                    close(fd);
            }
    }

    return openfds;
}

int main(int argc, char **argv) {
    fprintf(stdout, "Nb fd open : %i\n", count_fds());
    
    exit(EXIT_SUCCESS);
}
