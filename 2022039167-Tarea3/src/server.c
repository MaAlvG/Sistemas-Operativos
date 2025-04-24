#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <asm-generic/socket.h>
#include <semaphore.h>

#define PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    int server_fd;
    int thread_id;
} thread_args;

sem_t thread_semaphore; // Semaphore to track available threads

void* handle_client(void* arg) {
    thread_args* args = (thread_args*)arg;
    int new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    
    printf("Thread %d waiting for connections...\n", args->thread_id);
    
    while (1) {
        sem_wait(&thread_semaphore); // Wait for an available thread slot
        if ((new_socket = accept(args->server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            sem_post(&thread_semaphore); // Release the thread slot
            continue;
        }
        
        read(new_socket, buffer, BUFFER_SIZE);
        printf("Thread %d received request:\n%s\n", args->thread_id, buffer);
        
        // Simple HTTP response
        char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
        write(new_socket, response, strlen(response));
        close(new_socket);
        sem_post(&thread_semaphore); // Release the thread slot
    }
    
    return NULL;
}

int main(int argc, char const *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be greater than 0\n");
        exit(EXIT_FAILURE);
    }

    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Start listening
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Server started on port %d\n", PORT);
    
    // Initialize semaphore
    sem_init(&thread_semaphore, 0, num_threads);

    // Create worker threads
    pthread_t threads[num_threads];
    thread_args args[num_threads];
    
    for (int i = 0; i < num_threads; i++) {
        args[i].server_fd = server_fd;
        args[i].thread_id = i;
        pthread_create(&threads[i], NULL, handle_client, &args[i]);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    // Destroy semaphore
    sem_destroy(&thread_semaphore);

    return 0;
}