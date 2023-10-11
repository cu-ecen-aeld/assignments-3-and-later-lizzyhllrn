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

int sock_fd, client_fd;

static void signal_handler (int signal_number) {
  syslog(LOG_INFO, "Caught signal, exiting");
  remove("/var/tmp/aesdsocketdata");
  shutdown(sock_fd, SHUT_RDWR);
  shutdown(client_fd, SHUT_RDWR);

}


int main(int argc, char *argv[]) {
  
  //catch daemon flag(s)
  bool isDaemon = false;
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
  } //bind socket      
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


///////////////////////////// connection is set up, beginning main loop
  //continuously try to accept connections
  while (1) {
    ///// accepting client
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    client_fd = accept(sock_fd, (struct sockaddr*)&client_addr , &client_addr_len);
    if (client_fd == -1) {
      fprintf(stderr, "accept error:");
      return -1;
    }  
    
    // log IP of client if successful
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
     printf("Accepted connection from %s", client_ip);
    syslog(LOG_INFO, "Accepted connection from %s", client_ip);
    
    /////// opening file to write to
    FILE *fptr;
    fptr = fopen("/var/tmp/aesdsocketdata", "a+");
    //////// setting up buffer
    size_t buffer_size = 512;
    ssize_t recv_size;
    char* buffer = (char *)malloc(buffer_size * sizeof(char));
    memset(buffer, 0, buffer_size * sizeof(char));
    
    ///// loop while we are receiving data    
    while ( (recv_size = recv(client_fd, buffer, buffer_size, 0)) > 0)
    {
        if (recv_size == -1) {
          fprintf(stderr, "recv error");
        } else {
          // write received data to file
          fwrite(buffer, 1, recv_size, fptr);
        }
        
        if (buffer[recv_size-1]=='\n') //end of packet, send it back
        { 
          //break from main receving loop and send back?"
          break;
          // open the output data file as read only
          //FILE *fptr_read;
          //fptr_read = fopen("/var/tmp/aesdsocketdata", "r");
          
          //fseek(fptr, 0, SEEK_SET);

        //fclose(fptr_read);
        //move file pointer back
        //fseek(fptr, 0, SEEK_END);

        }
    }
    rewind(fptr);
    char* line = NULL;
    size_t len = 0;
    ssize_t read_size = 0;
    
    //read file and send back
    while ((read_size = getline(&line, &len, fptr))  !=-1) {
            printf("sending something\n");
            send(client_fd, line, read_size, 0);
    }
    free(line);
    fclose(fptr); 
  
  free(buffer);
  close(client_fd);
  syslog(LOG_INFO, "Closed connection from %s", client_ip);
  }
  
  //remove("/var/tmp/aesdsocketdata");
  close(sock_fd);
  
  
  return 0;

}
