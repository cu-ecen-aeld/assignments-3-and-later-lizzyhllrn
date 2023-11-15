#include "aesdsocket.h"
#include "../aesd-char-driver/aesd_ioctl.h"
#define USE_AESD_CHAR_DEVICE 1;

//Global variables
bool isDaemon;
bool isError=false;

//Linked list head
Node* head =NULL;

//File variables
FILE *file;
int file_fd;
#ifdef USE_AESD_CHAR_DEVICE
    #define DATA_FILE "/dev/aesdchar"

#else
    #define DATA_FILE "/var/tmp/aesdsocketdata"
#endif

int server_fd;

// Mutex to protect the linked list
pthread_mutex_t listMutex;
// Mutex to protect file access
pthread_mutex_t fileMutex;


int main(int argc, char *argv[]) {
  //int status;
  isDaemon=false;
  struct addrinfo hints;
  struct addrinfo *servinfo;
  struct sigaction new_action;
  //int ret;
  

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

    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create the server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return 1;
    }

    // Set options for reuseable address
    int opt = 1;
    if ((setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int))) == -1) {
        perror("socket options");
        return 1;
    }


    // Bind the server socket to a port
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Listening on port 9000\n");


    
    #ifndef USE_AESD_CHAR_DEVICE

    // Create a thread for timestamping
    pthread_t timestamp_thread;

    if (pthread_create(&timestamp_thread, NULL, timestamp, NULL) != 0) {
    perror("pthread_create for timestamp");
    return 1;
    }
    #endif

    pthread_mutex_init(&fileMutex, NULL);
    pthread_mutex_init(&listMutex, NULL);
    

    while (!isError) {
        // Accept a new client connection
        if ((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len)) == -1) {
            perror("accept");
            continue;
        }

        // Create a thread to handle the client
        thread_data_t* data = (thread_data_t*)malloc(sizeof(thread_data_t));
        data->client_fd = client_fd;
        data->isComplete = 0;

        if (pthread_create(&(data->threadId), NULL, client_handler, data) == 0) {
            insertNode(data);
        } else {
            perror("pthread_create");
            free(data);
        }

        // Check for and remove completed threads
        removeCompletedThreads();
    }

    // Close the server socket 
    do_shutdown();

    return 0;
}
    
    
void* client_handler(void *arg)
{
   thread_data_t* thread_data = (thread_data_t*)arg;
    
    // Handle the client connection, e.g., read and write data
    char buffer[1024];
    ssize_t bytes_received;
    bool full_cmd = false;
    char *ioctl_cmd;
    size_t bytes_written;
    ioctl_cmd = "AESDCHAR_IOCSEEKTO";
    memset(buffer, '\0', sizeof(buffer)); //clear buffer


    //while loop to receive whole command, write command to buffer
    while (!full_cmd) {
        bytes_received = recv(thread_data->client_fd, buffer, sizeof(buffer), 0);
        if (buffer[bytes_received-1]=='\n') {
            full_cmd = true;
            break;
        }
        if (bytes_received == 0) {
            printf("closed connection\n");
            break;
        }
    }
    printf("recieved full cmd: %s\n and bytes_recieved is %ld\n", buffer, bytes_received);


    if(full_cmd){ //when the full command is recieved, write it to buffer
        printf("in full command\n");

        bool ioctl_found = (strstr(buffer, ioctl_cmd) != NULL); //Check if the command is ioctl


        if (ioctl_found) {
            printf("found ioctl command\n");
            struct aesd_seekto seekto;
            sscanf(buffer, "AESDCHAR_IOCSEEKTO:%d,%d", &seekto.write_cmd, &seekto.write_cmd_offset); //Populate seekto structure
            printf("seek to at %d and %d\n", seekto.write_cmd, seekto.write_cmd_offset);

            if (pthread_mutex_lock(&fileMutex) !=0) {
                printf("error with mutex lock\n");
            } // Lock for file access

            file_fd = open(DATA_FILE,  O_RDWR ,0666); // open data file

            if(file_fd == -1 ) {
                printf("file error\n");
                fprintf(stderr, "file open error: %d\n", errno);
            }

            int ioctl_ret = ioctl(file_fd, AESDCHAR_IOCSEEKTO, &seekto); // ioctl on file
            if(ioctl_ret < 0) {
                printf("ioctl error %d", ioctl_ret);
            }
            //keep file open for send back

        } else { //write command to file
            printf("ioctl not found, attempting to write...\n");
            if (pthread_mutex_lock(&fileMutex) !=0) {
                printf("error with mutex lock\n");
            } // Lock for file access
            file_fd = open(DATA_FILE, O_CREAT | O_RDWR | O_APPEND ,0666); //open file for write
            
            if (file_fd == -1) {
                fprintf(stderr, "file open error: %d\n", errno);
            }
            bytes_written = write(file_fd, buffer, bytes_received);
            if( bytes_written == -1 ) {
                fprintf(stderr, "write error: %d\n", errno);
            }
            printf("wrote %ld bytes\n", bytes_written);
            close(file_fd);
            pthread_mutex_unlock(&fileMutex); // Unlock file access

            //fclose(file);
            //pthread_mutex_unlock(&fileMutex); // Unlock file access

        }
        if(!ioctl_found) { //reopen closed file to send back
            if (pthread_mutex_lock(&fileMutex) !=0) {
                printf("error with mutex lock\n");
            } // Lock for file access
            file_fd = open(DATA_FILE, O_RDONLY, 0644);
        }

        memset(buffer, '\0', sizeof(buffer)); //clear buffer
        int bytes_read; 
        int sent_bytes = 0;

        while (1) {
            
            bytes_read = read(file_fd, buffer, sizeof(buffer));
            if (bytes_read == -1) {
                printf("error reading file\n");
                break;
            }
            if (bytes_read == 0) {
                printf("reached eof\n");
                break;
            }

            sent_bytes= send(thread_data->client_fd, buffer, bytes_read, 0);
            printf("send %d bytes\n", sent_bytes);
            
        }

        close(file_fd);
        pthread_mutex_unlock(&fileMutex); // Unlock file access
        
    }

    // Mark the thread as complete
    thread_data->isComplete = 1;

    close(thread_data->client_fd);
    return NULL;
}

static void signal_handler (int signal_number) {
  syslog(LOG_INFO, "Caught signal, exiting");
  isError=true;
  
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


void* timestamp(void * arg){
    while (1) {
        sleep(10); // Wait for 10 seconds

        pthread_mutex_lock(&fileMutex); // Lock for file access
        FILE* file = fopen(DATA_FILE, "a+");
        if (file != NULL) {
            time_t rawtime;
            struct tm* timeinfo;

            // Get current time
            time(&rawtime);
            timeinfo = localtime(&rawtime);

            char timestamp[30];
            // Format timesteamp
            strftime(timestamp, sizeof(timestamp),"%a, %d %b %Y %H:%M:%S %z", timeinfo);

            fprintf(file, "timestamp: %s\n", timestamp);
            fclose(file);
        }
        pthread_mutex_unlock(&fileMutex); // Unlock file access
    }

}


void do_shutdown(void) {
    //join threads
    pthread_mutex_lock(&listMutex);
    Node* current = head;
    Node* prev = NULL;

    while (current != NULL) {
        // Join and cleanup the thread
        pthread_join(current->data->threadId, NULL);

        // Remove the node from the list
        if (prev == NULL) {
            head = current->next;
            free(current->data);
            free(current);
            current = head;
        } else {
            prev->next = current->next;
            free(current->data);
            free(current);
            current = prev->next;
        }
    }

    pthread_mutex_unlock(&listMutex);

    pthread_mutex_destroy(&fileMutex);
    pthread_mutex_destroy(&listMutex);
    //cleanup sockets
    close(server_fd);

    //cleanup files
    if (file !=NULL){
        fclose(file); 

    }
#ifndef USE_AESD_CHAR_DEVICE
    remove(DATA_FILE);
#endif
}


void insertNode(thread_data_t* data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;

    pthread_mutex_lock(&listMutex);

    if (head == NULL) {
        head = newNode;
    } else {
        Node* current = head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }

    pthread_mutex_unlock(&listMutex);
}

void removeCompletedThreads() {
    pthread_mutex_lock(&listMutex);

    Node* current = head;
    Node* prev = NULL;

    while (current != NULL) {
        if (current->data->isComplete) {
            // Join and cleanup the thread
            pthread_join(current->data->threadId, NULL);

            // Remove the node from the list
            if (prev == NULL) {
                head = current->next;
                free(current->data);
                free(current);
                current = head;
            } else {
                prev->next = current->next;
                free(current->data);
                free(current);
                current = prev->next;
            }
        } else {
            prev = current;
            current = current->next;
        }
    }

    pthread_mutex_unlock(&listMutex);
}