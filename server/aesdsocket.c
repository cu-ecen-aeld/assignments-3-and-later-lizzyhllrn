#include "aesdsocket.h"

//Global variables
bool isDaemon;

//Socket variables
int sock_fd, client_fd;

//Thread variables
pthread_mutex_t lock;
thread_data_t time_thread_data;
SLIST_HEAD(slisthead, slist_data_s) head;
slist_data_t * slist_data_s =NULL;

//File variables
FILE *fptr;

int main(int argc, char *argv[]) {
  int status;
  isDaemon=false;
  struct addrinfo hints;
  struct addrinfo *servinfo;
  struct sigaction new_action;
  int ret;
  
  ret = pthread_mutex_init(&lock, NULL);
  if(ret !=0){
    fprintf(stderr, "mutex init failed\n");
    printf("mutex init failed\n");
    return -1;
  }

  
  //Catch daemon flag(s)
  if (argc > 1 && strcmp(argv[1], "-d") == 0)
  {
    printf("it's a Daemon\n");
    isDaemon = true;
  }
  
  //Set up the signal handling
  memset(&new_action, 0, sizeof(struct sigaction));
  new_action.sa_handler=signal_handler;
  if (sigaction(SIGTERM, &new_action, NULL) != 0) {
    fprintf(stderr, "Error %d registering for SIGTERM", errno);
  } 
  if (sigaction(SIGINT, &new_action, NULL) != 0) {
    fprintf(stderr, "Error %d registering for SIGINT", errno);
  }


  
  //set up sockaddr using getaddrinfo
  memset(&hints, 0, sizeof(hints)); //make sure struct is empty
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol=0;
  if ((status = getaddrinfo(NULL, "9000", &hints, &servinfo)) != 0)
  {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));

    return -1;
  }

  if (servinfo == NULL) {
    printf("getaddrinfo memory allocation failed\n");
    return -1;
  }
   
  //open streaming socket bound to port 9000 (specified in getaddrinfo) 
  sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if (sock_fd == -1)
  {
    fprintf(stderr, "socket error:");
    freeaddrinfo(servinfo);
    return -1;
  } 
  printf("opened socket successfully\n");
    //set reuseable socket

  int option = 1;
  if ((status = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int))) != 0) {
    fprintf(stderr, "options error: %s\n", gai_strerror(status));
    freeaddrinfo(servinfo);
    return -1;
  } 
  printf("set socket options\n");
  
  //bind socket      
  if ((status = bind(sock_fd, servinfo->ai_addr , servinfo->ai_addrlen)) != 0)
  {
    fprintf(stderr, "bind error: %s\n", gai_strerror(status));
    freeaddrinfo(servinfo);
    return -1;
  }  
  freeaddrinfo(servinfo);
  
  /// handle the Daemon flag
  if (isDaemon) {
    if(make_Daemon()==-1)
    {
      return -1;
    }
  }

  // beginning listening on socket
  if ((status = listen(sock_fd, 1)) != 0)
  {
    fprintf(stderr, "listen error: %s\n", gai_strerror(status));
    return -1;
  } 
  printf("listening...\n");
  
      
  //initialize s lsit
  SLIST_INIT(&head);


  
  //begin timestamping, create timestamping thread
  //time_thread_data = malloc(sizeof(thread_data_t));
  time_thread_data.pMutex = &lock;
  /*if (pthread_create(&(time_thread_data.threadId), NULL, timestamp, &time_thread_data)==-1)
  {
    fprintf(stderr, "error creating thread\n");
    return -1;
  }
  */
  
struct sockaddr_in client_addr;
socklen_t client_addr_len = sizeof(client_addr);
void * thread_return = NULL;

slist_data_t *datap;

  //continuously try to accept connections
  printf("set up successful, will listen for connections\n");
while (1) {
  ///// accepting client    
  
  client_fd = accept(sock_fd, (struct sockaddr*)&client_addr , &client_addr_len);
  if (client_fd == -1) {
    fprintf(stderr, "accept error:");
    //free(datap);
    return -1;
  }  
  printf("accepted a connection\n");
  

  //start 6-1 linked list of threads for each accept
  //track thread ids
  // for each thread in list is complete flag set, if yes then join
    datap = malloc(sizeof(slist_data_t));
    datap->isComplete = false;
    datap->pMutex = &lock;
    datap->client_fd = client_fd;
    datap->client_addr = client_addr;
    
    if (pthread_create(&(datap->threadId), NULL, data_handler, &datap)==-1)
    {
      fprintf(stderr, "error creating thread\n");
      free(datap);
      return -1;
    }
    printf("created a thread\n");
    
    SLIST_INSERT_HEAD(&head, datap, entries);
    datap = NULL;
    
    SLIST_FOREACH(datap, &head, entries)
    {
      if (datap->isComplete)
      {
        if (pthread_join(datap->threadId, &thread_return)==-1)
        {
            fprintf(stderr, "error joining thread\n");
            free(datap);
            return -1;
        }
        free(datap);
      }
    
    }
    
}
  
  do_shutdown();
  
  return 0;
}
    
    
void* data_handler(void *thread_param)
{
   printf("beginning data handler routine\n");
  slist_data_t *thread_data= (slist_data_t*)thread_param;
    // log IP of client if successful
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(thread_data->client_addr.sin_family), client_ip, INET_ADDRSTRLEN);
    printf("Accepted connection from %s\n", client_ip);
    syslog(LOG_INFO, "Accepted connection from %s", client_ip);
    
    //////// setting up buffer
    size_t buffer_size = 512;
    ssize_t recv_size;
    char* buffer = (char *)malloc(buffer_size * sizeof(char));
    memset(buffer, 0, buffer_size * sizeof(char));

      //open file to write to
    fptr = fopen("/var/tmp/aesdsocketdata", "a+");
    
    ///// loop while we are receiving data, lock file while handling
    if (pthread_mutex_lock(thread_data->pMutex)==-1)
    {
        fprintf(stderr, "error with mutex lock\n");
        free(buffer);
        close(thread_data->client_fd);
        return NULL;
    }
    printf("locked the mutex to write\n");

   /* 
    recv_size = recv(thread_data->client_fd, buffer, buffer_size, 0);
    if (recv_size == -1) {
     printf("error with recv: %s\n", strerror(errno));
      fprintf(stderr, "recv error");
    }

    printf("recieved %ld bytes", recv_size);
    while ( (recv_size  > 0))
    {
        printf("in recieiving while function\n");
        if (recv_size == -1) {
          printf("error with recv");
          fprintf(stderr, "recv error");
        } else {
          // write received data to file
          printf("recieved and wrote some data\n");
          fwrite(buffer, 1, recv_size, fptr);
        }
        
        if (buffer[recv_size-1]=='\n') //end of packet, send it back
        { 
          //break from main receving loop and send back?"
          printf("reached end of packet, breaking main loop\n");
          break;
        }
    }
    */
    if (pthread_mutex_unlock(thread_data->pMutex)==-1)
    {
        fprintf(stderr, "error with mutex lock\n");
        close(thread_data->client_fd);
        free(buffer);
        return NULL;
    }
    printf("unlocked for writing\n");
    rewind(fptr);
    char* line = NULL;
    size_t len = 0;
    ssize_t read_size = 0;
    
    //read file and send back
    
    if (pthread_mutex_lock(thread_data->pMutex)==-1)
    {
        fprintf(stderr, "error with mutex lock\n");
        free(buffer);
        close(thread_data->client_fd);
        return NULL;
    } printf("locked for reading\n");
    while ((read_size = getline(&line, &len, fptr))  !=-1) {
            printf("sending something\n");
            send(thread_data->client_fd, line, read_size, 0);
    }
    if (pthread_mutex_unlock(thread_data->pMutex)==-1)
    {
        fprintf(stderr, "error with mutex lock\n");
        free(buffer);
        close(thread_data->client_fd);
        return NULL;
    } printf("unlocked for reading\n");
    free(line);
    
  thread_data->isComplete=true;
  free(buffer);
  //close(thread_data->client_fd);
  if (shutdown(thread_data->client_fd, SHUT_RDWR) ==-1){
    printf("error with shutdown: %s\n", strerror(errno));
  }
  syslog(LOG_INFO, "Closed connection from %s", client_ip);
  printf("made it to end of thread");
  return NULL;

}

static void signal_handler (int signal_number) {
  syslog(LOG_INFO, "Caught signal, exiting");
  do_shutdown();
}

int make_Daemon(void) {
    pid_t pid = fork();
    if (pid < 0) {
      fprintf(stderr, "couldn't fork");
      return -1;
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
    return 0;
}


void* timestamp(void * thread_param){
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[30];
    thread_data_t *time_t_data = (thread_data_t *)thread_param;
  
    while(1) {
      //get updated time
      time(&rawtime);
      timeinfo = localtime(&rawtime);
      strftime(timestamp, sizeof(timestamp),"%a, %d %b %Y %H:%M:%S %z", timeinfo);

      if (pthread_mutex_lock(time_t_data->pMutex)==-1)
      {
          fprintf(stderr, "error with mutex lock (in time func)\n");
          return NULL;
      }
      
      
      fprintf(fptr, "timestamp: %s\n", timestamp);
      
      if (pthread_mutex_unlock(time_t_data->pMutex)==-1)
      {
          fprintf(stderr, "error with mutex unlock (in time func)\n");
          return NULL;
      }
      sleep(10);
    
    }
    return NULL;

}

void do_shutdown(void) {
  //join threads
  slist_data_t *datap;
  void * thread_return = NULL;
  if (pthread_join(time_thread_data.threadId, &thread_return)==-1)
  {
    fprintf(stderr, "error joining timestamp thread\n");
  }
  //free(time_thread_data);
  SLIST_FOREACH(datap, &head, entries)
  {
    close(datap->client_fd);
    if (pthread_join(datap->threadId, &thread_return)==-1)
    {
      fprintf(stderr, "error joining thread\n");
    }
    free(datap);
  }
  //cleanup sockets
  close(sock_fd);
  close(client_fd);

  //cleanup files
  fclose(fptr); 
  remove("/var/tmp/aesdsocketdata");
}


