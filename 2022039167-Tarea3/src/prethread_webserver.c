#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    int server_fd;
    int thread_id;
} thread_args;

sem_t thread_semaphore; // Semaphore to track available threads

void get_request(int client_socket, const char* request) {
    // Example GET response
    char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
    write(client_socket, response, strlen(response));
}

void post_request(int client_socket, const char* request) {
    // Example POST response
    char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 7\n\nPosted!";
    write(client_socket, response, strlen(response));
}

void head_request(int client_socket, const char* request) {
    // Example HEAD response (headers only, no body)
    char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 0\n\n";
    write(client_socket, response, strlen(response));
}

void put_request(int client_socket, const char* request) {
    // Example PUT response
    char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 6\n\nUpdated";
    write(client_socket, response, strlen(response));
}

void delete_request(int client_socket, const char* request) {
    // Example DELETE response
    char *response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 7\n\nDeleted";
    write(client_socket, response, strlen(response));
}

void handle_request(int client_socket, const char* request) {
    char method[8]; // To store the HTTP method (e.g., GET, POST)
    sscanf(request, "%s", method); // Extract the method from the request

    if (strcmp(method, "GET") == 0) {
        get_request(client_socket, request);
    } else if (strcmp(method, "POST") == 0) {
        post_request(client_socket, request);
    } else if (strcmp(method, "HEAD") == 0) {
        head_request(client_socket, request);
    } else if (strcmp(method, "PUT") == 0) {
        put_request(client_socket, request);
    } else if (strcmp(method, "DELETE") == 0) {
        delete_request(client_socket, request);
    } else {
        // Handle unsupported methods
        char *response = "HTTP/1.1 405 Method Not Allowed\nContent-Length: 0\n\n";
        write(client_socket, response, strlen(response));
    }
}

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
        
        handle_request(new_socket, buffer); // Delegate to handle_request()
        
        close(new_socket);
        sem_post(&thread_semaphore); // Release the thread slot
    }
    
    return NULL;
}

//-n hils -w httproot -p puerto
int main(int argc, char *argv[]) {
    
    /*
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }*/

    int arg_opt;
    int num_threads = 10;
    char *direction = NULL;
    int port = DEFAULT_PORT;
    extern char *optarg;
    extern int optind;

    while ((arg_opt = getopt(argc, argv, "n:w:p:")) != -1) {
        switch (arg_opt) {
            case 'n':
                num_threads = atoi(optarg);
                break;
            case 'w':
                direction = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Uso: %s [-n hilos] [-W direccion] [-p puerto]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    fprintf(stderr, "Uso: %s [-n hilos] [-W direccion] [-p puerto]\n", argv[0]);
    /*
    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be greater than 0\n");
        exit(EXIT_FAILURE);
    }*/

    int server_fd;
    struct sockaddr_in address;
    int opt_val = 1;
    int addrlen = sizeof(address);
    
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt_val, sizeof(opt_val))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
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
    
    printf("Server started on port %d\n", port);
    
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