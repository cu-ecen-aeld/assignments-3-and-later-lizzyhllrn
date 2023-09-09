// This writes a string to file specified by input
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>

int main(int argc, char *argv[]) {
  const char * writefile = argv[1];
  const char * writestr = argv[2];
  
  openlog("assignment-2-log", LOG_PERROR, LOG_USER);
  
  if(writefile == NULL | writestr == NULL)
  {
  syslog(LOG_ERR, "Incorrect number of inputs");
  exit(1);
  }
  
  FILE *fptr;
  fptr = fopen(writefile, "w");
  
  if(fptr == NULL)
  {
  syslog(LOG_ERR, "Error accessing file");
  exit(1);
  
  }
  
  
  fprintf(fptr, "%s", writestr);
  syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);
  
  fclose(fptr);
  
  return 0;

}

