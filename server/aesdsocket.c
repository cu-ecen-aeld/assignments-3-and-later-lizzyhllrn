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

/*
struct sockadder {
      sa_family_t sa_family;
      char sa_data[14];
    };
*/

int main(int argc, char *argv[]) {
  
int status;
struct addrinfo hints;
struct addrinfo *servinfo;

memset(&hints, 0, sizeof(hints)); //make sure struct is empty
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = AI_PASSIVE;

if ((status = getaddrinfo(NULL, "9000", &hints, &servinfo)) != 0)
{
  fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
  return -1;
}
 
  int sock_fd = socket(servinfo->ai_family, servinfo->ai_socktype, 0);
  if (sock_fd == -1)
{
  fprintf(stderr, "socket error:");
  return -1;
}  
    
if ((status = bind(sock_fd, servinfo->ai_addr , servinfo->ai_addrlen)) != 0)
{
  fprintf(stderr, "bind error: %s\n", gai_strerror(status));
  return -1;
}  

if ((status = listen(sock_fd, 1)) != 0)
{
  fprintf(stderr, "listen error: %s\n", gai_strerror(status));
  return -1;
}  

struct sockaddr_in client_addr;
socklen_t client_addr_len = sizeof(client_addr);
int client_fd = accept(sock_fd, (struct sockaddr*)&client_addr , &client_addr_len);
if (client_fd == -1)
{
  fprintf(stderr, "accept error:");
  return -1;
}  

  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
  fprintf("Accepted connection from %s", client_ip);
  syslog(LOG_INFO, "Accepted connection from %s", client_ip);


  freeaddrinfo(servinfo);
  return 0;

}
