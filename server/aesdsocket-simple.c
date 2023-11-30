#include "aesdsocket.h"
#include "../aesd-char-driver/aesd_ioctl.h"
#include "capture.h"
//#define USE_AESD_CHAR_DEVICE 1;
#define BUF_LEN 1024

#ifdef USE_AESD_CHAR_DEVICE
    #define DATA_FILE "/dev/aesdchar"

#else
    #define DATA_FILE "/var/tmp/aesdsocketdata"
#endif

int server_fd;
//Global variables
bool isDaemon;
bool isError=false;

//Linked list head
Node* head =NULL;
//File variables
//FILE *file;
int file_fd;

// Mutex to protect the linked list
pthread_mutex_t listMutex;
// Mutex to protect file access
pthread_mutex_t fileMutex;


int capture_motion(void);

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


    capture_motion();
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
    char receive_buffer[BUF_LEN];
    char send_buffer[BUF_LEN];
    size_t bytes_read; 
    size_t sent_bytes = 0;
    //int file_fd;
    ssize_t bytes_received =0;
    bool full_cmd = false;
    size_t bytes_written;
    int motionDetected;
    //char* returnString = "Motion Detected";

    memset(receive_buffer, '\0', BUF_LEN); //clear buffer
    memset(send_buffer, '\0', BUF_LEN); //clear buffer



    //while loop to receive whole command, write command to buffer
    while (!full_cmd) {
        bytes_received = recv(thread_data->client_fd, receive_buffer, BUF_LEN, 0);
        if (bytes_received == -1) {
            fprintf(stderr, "receive error");
            break;
        }

        if (bytes_received == 0) {
            printf("closed connection\n");
            break;
        }

        if (pthread_mutex_lock(&fileMutex) !=0) {
            printf("error with mutex lock\n");
        } // Lock for file access

        file_fd = open(DATA_FILE, O_CREAT | O_RDWR | O_APPEND ,0666); //open file for write
        if (file_fd == -1) {
            fprintf(stderr, "file open error: %d\n", errno);
        }

        bytes_written = write(file_fd, receive_buffer, bytes_received);
        if( bytes_written == -1 ) {
            fprintf(stderr, "write error: %d\n", errno);
        }

        close(file_fd);
        pthread_mutex_unlock(&fileMutex); // Unlock file access

       
        if (receive_buffer[bytes_received-1]=='\n') {
            full_cmd = true;
            printf("newline found, breaking while loop\n");
            break;
        }
    }
    printf("recieved full cmd: %s\n and bytes_recieved is %ld\n", receive_buffer, bytes_received);


    if (pthread_mutex_lock(&fileMutex) !=0) {
        printf("error with mutex lock\n");
    } // Lock for file access

    file_fd = open(DATA_FILE, O_RDONLY, 0644);
    if (file_fd == -1) {
        fprintf(stderr, "file open error: %d\n", errno);
    }

    while (1) {
        bytes_read = read(file_fd, send_buffer, BUF_LEN);
        if (bytes_read == -1) {
            printf("error reading file\n");
            break;
        }
        if (bytes_read == 0) {
            printf("reached eof\n");
            break;
        }

        pthread_mutex_unlock(&fileMutex); // Unlock file access after reading
        
        sent_bytes= send(thread_data->client_fd, send_buffer, bytes_read, 0);
        if (sent_bytes == -1) {
            fprintf(stderr, "sent error: %d\n", errno);
        }
        printf("sent %ld bytes of %s\n", sent_bytes, send_buffer);
            
    }
    motionDetected = capture_motion();
    
    if (motionDetected)
    {
        //sent_bytes= send(thread_data->client_fd, returnString, sizeof(returnString), 0);
        printf("motion detected\n");

    }

    close(file_fd);
    pthread_mutex_unlock(&fileMutex); // Unlock file access

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
    //if (file !=NULL){
    //    fclose(file); 

    //}
    if (close(file_fd)==-1) {
        fprintf(stderr, "failed to close file\n");
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