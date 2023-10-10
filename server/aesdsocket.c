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


int main(int argc, char *argv[]) {
  
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
  int sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, 0);
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

  // beginning listening on socket
  if ((status = listen(sock_fd, 1)) != 0)
  {
    fprintf(stderr, "listen error: %s\n", gai_strerror(status));
    return -1;
  }  

  //continuously try to accept connections
  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    int client_fd = accept(sock_fd, (struct sockaddr*)&client_addr , &client_addr_len);
    if (client_fd == -1) {
      fprintf(stderr, "accept error:");
      return -1;
    }  
    
    // log IP of client if successful
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
     printf("Accepted connection from %s", client_ip);
    syslog(LOG_INFO, "Accepted connection from %s", client_ip);
    
    FILE *fptr;
    fptr = fopen("/var/tmp/aesdsocketdata", "w");

    size_t buffer_size = 512;
    ssize_t recv_size;
    char* buffer = (char *)malloc(buffer_size * sizeof(char));
    memset(buffer, 0, buffer_size * sizeof(char));
    // receive data packet and write until to file until newline received
    while ( (recv_size = recv(client_fd, buffer, buffer_size, 0)) > 0)
    {
        if (recv_size == -1) {
          fprintf(stderr, "recv error");
        } else {
          fwrite(buffer, 1, recv_size, fptr);
        }
        
        if (buffer[recv_size-1]=='\n') //end of packet, send it back
        {
          fseek(fptr, 0, SEEK_SET);
          char* line = NULL;
          size_t len = 0;
          ssize_t read = 0;
          while ((read=getline(&line, &len, fptr)) != -1) {
            send(client_fd, line, read, 0);
          }
          free(line);
        }
      
  }


  
  
  free(buffer);
  close(client_fd);
  syslog(LOG_INFO, "Closed connection from %s", client_ip);
  }
  fclose(fptr);
  remove("/var/tmp/aesdsocketdata");
  close(sock_fd);
  freeaddrinfo(servinfo);
  
  return 0;

}
