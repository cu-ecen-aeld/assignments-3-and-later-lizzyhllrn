#define _GNU_SOURCE


#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/queue.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in.h>

//A6-1 additions
//this structure contains linked list data for each thread that is split off for new connection
typedef struct {
    pthread_t threadId;
    pthread_mutex_t *pMutex;
} thread_data_t;

typedef struct slist_data_s slist_data_t;

struct slist_data_s {
    bool isComplete;
    pthread_t threadId;
    pthread_mutex_t *pMutex;
    struct sockaddr_in client_addr;
    int client_fd;
    SLIST_ENTRY(slist_data_s) entries;
} s_list_data_t;



//thread functions
void* data_handler(void *thread_param);
void* timestamp(void *thread_param);

int make_Daemon(void);
static void signal_handler (int signal_number);
void do_shutdown(void);
