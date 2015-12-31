#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/mman.h>

#include "common_impl.h"
#include "common_net.h"


/* fin des includes */

#define TOP_ADDR    (0x40000000)
#define PAGE_NUMBER (100)
#define PAGE_SIZE   (sysconf(_SC_PAGE_SIZE))
#define BASE_ADDR   (TOP_ADDR - (PAGE_NUMBER * PAGE_SIZE))

typedef enum
{
   NO_ACCESS, 
   READ_ACCESS,
   WRITE_ACCESS, 
   UNKNOWN_ACCESS 
} dsm_page_access_t;

typedef enum
{   
   INVALID,
   READ_ONLY,
   WRITE,
   NO_CHANGE  
} dsm_page_state_t;

typedef int dsm_page_owner_t;

typedef struct
{ 
   dsm_page_state_t status;
   dsm_page_owner_t owner;
} dsm_page_info_t;

dsm_page_info_t table_page[PAGE_NUMBER];

// Tableau des structures des processus dsmwrap
dsm_proc_t *proc_array;

// On ne le protège pas par verrou, cette variable changera de valeurs
// une seule fois.
extern bool finalization;

pthread_t comm_daemon;
extern int DSM_NODE_ID;
extern int dsm_node_num;

char *dsm_init( int argc, char **argv);
void  dsm_finalize( void );
