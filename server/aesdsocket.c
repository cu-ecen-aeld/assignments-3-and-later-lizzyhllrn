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

int sock_fd, client_fd;

//A6-1 additions
// SLIST.
typedef struct slist_data_s slist_data_t;
struct slist_data_s {
    bool isComplete;
    pthread_t threadId;
    pthread_mutex_t *pMutex;
    struct sockaddr_in client_addr;
    SLIST_ENTRY(slist_data_s) entries;
};

pthread_mutex_t lock;

void* data_handler(void *thread_param);


static void signal_handler (int signal_number) {
  syslog(LOG_INFO, "Caught signal, exiting");
  remove("/var/tmp/aesdsocketdata");
  shutdown(sock_fd, SHUT_RDWR);
  shutdown(client_fd, SHUT_RDWR);

}


int main(int argc, char *argv[]) {
  
  //catch daemon flag(s)
  bool isDaemon = false;
  pthread_mutex_init(&lock, NULL);
  if (argc > 1 && strcmp(argv[1], "-d") == 0)
  {
    printf("it's a Daemon\n");
    isDaemon = true;
  }
  
  //set up the signal handling
  struct sigaction new_action;
  memset(&new_action, 0, sizeof(struct sigaction));
  new_action.sa_handler=signal_handler;
  if (sigaction(SIGTERM, &new_action, NULL) != 0) {
    fprintf(stderr, "Error %d registering for SIGTERM", errno);
  } 
  if (sigaction(SIGINT, &new_action, NULL) != 0) {
    fprintf(stderr, "Error %d registering for SIGINT", errno);
  }
  
  
  int status;
  struct addrinfo hints;
  struct addrinfo *servinfo;

  //set up sockaddr using getaddrinfo
  memset(&hints, 0, sizeof(hints)); //make sure struct is empty
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((status = getaddrinfo(NULL, "9000", &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    return -1;
  }
 
  //open streaming socket bound to port 9000 (specified in getaddrinfo) 
  sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, 0);
  if (sock_fd == -1)
  {
    fprintf(stderr, "socket error:");
    return -1;
  } 
    //set reuseable socket
  int option = 1;
  if ((status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int))) != 0) {
    fprintf(stderr, "options error: %s\n", gai_strerror(status));
    return -1;
  } 
  
  //bind socket      
  if ((status = bind(sock_fd, servinfo->ai_addr , servinfo->ai_addrlen)) != 0)
  {
    fprintf(stderr, "bind error: %s\n", gai_strerror(status));
    return -1;
  }  
  
  /// handle the Daemon flag
  if (isDaemon) {
    pid_t pid = fork();
    if (pid < 0) {
      fprintf(stderr, "couldn't fork");
    }
    if (pid > 0) 
    { //we're in the parent so exit
      exit(0);
    }
    // if in child, create new SID
    setsid();
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
  }

  // beginning listening on socket
  if ((status = listen(sock_fd, 1)) != 0)
  {
    fprintf(stderr, "listen error: %s\n", gai_strerror(status));
    return -1;
  } 
  freeaddrinfo(servinfo);
      
    //initialize s lsit
    slist_data_t *datap=NULL;
    SLIST_HEAD(slisthead, slist_data_s) head;
    SLIST_INIT(&head);


///////////////////////////// connection is set up, beginning main loop
  //continuously try to accept connections
  while (1) {
    ///// accepting client
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    void * threadRetVal = NULL;
    
    client_fd = accept(sock_fd, (struct sockaddr*)&client_addr , &client_addr_len);
    if (client_fd == -1) {
      fprintf(stderr, "accept error:");
      return -1;
    }  
    
    //start 6-1 linked list of threads for each accept
    //track thread ids
    //TODO create thread
    // for each thread in list is complete flag set, if yes then join
      datap = malloc(sizeof(slist_data_t));
      datap->isComplete = false;
      datap->pMutex = &lock;
      datap->client_addr = client_addr;
      
      if (pthread_create(&(datap->threadId), NULL, data_handler, &datap)==-1)
      {
        fprintf(stderr, "error creating thread\n");
        free(datap);
        return -1;
      }
      
      SLIST_INSERT_HEAD(&head, datap, entries);
      datap = NULL;
      
      SLIST_FOREACH(datap, &head, entries)
      {
        if (datap->isComplete)
        {
          if (pthread_join(datap->threadId, &threadRetVal)==-1)
          {
              fprintf(stderr, "error joining thread\n");
              return -1;
          }
          free(datap);
        }
      
      }
      
  }
  
  return 0;
}
    
    
void* data_handler(void *thread_param)
{
  slist_data_t *thread_data= (slist_data_t*)thread_param;
    // log IP of client if successful
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(thread_data->client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
     printf("Accepted connection from %s", client_ip);
    syslog(LOG_INFO, "Accepted connection from %s", client_ip);
    
    /////// opening file to write to
    //TODO mutex for aesdsocketdata"
    
    FILE *fptr;
    fptr = fopen("/var/tmp/aesdsocketdata", "a+");
    //////// setting up buffer
    size_t buffer_size = 512;
    ssize_t recv_size;
    char* buffer = (char *)malloc(buffer_size * sizeof(char));
    memset(buffer, 0, buffer_size * sizeof(char));
    
    ///// loop while we are receiving data, lock file while handling
    if (pthread_mutex_lock(thread_data->pMutex)==-1)
    {
        fprintf(stderr, "error with mutex lock\n");
        return NULL;
    }
    while ( (recv_size = recv(client_fd, buffer, buffer_size, 0)) > 0)
    {
        if (recv_size == -1) {
          fprintf(stderr, "recv error");
        } else {
          // write received data to file
          printf("recieved and wrote some data\n");
          fwrite(buffer, 1, recv_size, fptr);
        }
        
        if (buffer[recv_size-1]=='\n') //end of packet, send it back
        { 
          //break from main receving loop and send back?"
          break;


        }
    }
    if (pthread_mutex_unlock(thread_data->pMutex)==-1)
    {
        fprintf(stderr, "error with mutex lock\n");
        return NULL;
    }
    rewind(fptr);
    char* line = NULL;
    size_t len = 0;
    ssize_t read_size = 0;
    
    //read file and send back
    
    if (pthread_mutex_lock(thread_data->pMutex)==-1)
    {
        fprintf(stderr, "error with mutex lock\n");
        return NULL;
    }
    while ((read_size = getline(&line, &len, fptr))  !=-1) {
            printf("sending something\n");
            send(client_fd, line, read_size, 0);
    }
    if (pthread_mutex_unlock(thread_data->pMutex)==-1)
    {
        fprintf(stderr, "error with mutex lock\n");
        return NULL;
    }
    free(line);
    fclose(fptr); 
    thread_data->isComplete=true;
  
  free(buffer);
  close(client_fd);
  syslog(LOG_INFO, "Closed connection from %s", client_ip);
  close(sock_fd);

}
