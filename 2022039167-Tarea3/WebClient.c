#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define THREAD_POOL_SIZE 5
#define BUFFER_SIZE 1024

// Structure to hold thread pool and related data
typedef struct {
    pthread_t *threads;
    int thread_count;
    int socket_fd;
    pthread_mutex_t mutex;
} ThreadPool;

// Function to handle client requests
void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[BUFFER_SIZE] = {0};
    int valread;

    // Read the request from the client
    valread = read(client_fd, buffer, BUFFER_SIZE);
    if (valread < 0) {
        perror("read");
        close(client_fd);
        return NULL;
    }
    
    // Simple response
    char *response = "HTTP/1.1 200 OK\r\nContent-Length: 12\r\n\r\nHello, World!";
    send(client_fd, response, strlen(response), 0);
    close(client_fd);
    return NULL;
}

// Function to add a client socket to the thread pool for processing
void add_client_to_pool(ThreadPool *pool, int client_fd) {
    pthread_t thread;
    if (pthread_create(&thread, NULL, handle_client, &client_fd) != 0) {
        perror("pthread_create");
        close(client_fd);
    }
    pthread_detach(thread); // Detach the thread so it cleans up automatically after it finishes
}

// Function to set up the server socket
int setup_server_socket() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    return server_fd;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    ThreadPool thread_pool;
    
    // Initialize the thread pool
    thread_pool.thread_count = 0;
    thread_pool.socket_fd = setup_server_socket();
    pthread_mutex_init(&thread_pool.mutex, NULL);
    
    printf("Server started, listening on port %d\n", PORT);

    // Accept incoming connections and add them to the thread pool
    while (1) {
        if ((client_fd = accept(thread_pool.socket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }
        add_client_to_pool(&thread_pool, client_fd);
    }
    
    // Cleanup
    close(server_fd);
    pthread_mutex_destroy(&thread_pool.mutex);
    return 0;
}