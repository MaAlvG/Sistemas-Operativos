#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 1024

typedef struct {
    int server_fd;
    int thread_id;
    int port;
} thread_args;

sem_t thread_semaphore; // Semaphore to track available threads

int file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

// GET: Read and return the content of a file
void get_request(int client_socket, const char* request) {
    char base_directory[6] = "./data";
    char file_path[1000] = "./data"; // Base directory
    int result = sscanf(request, "GET %s ", file_path + 6);

    if (file_exists(file_path)) {
        FILE *file = fopen(file_path, "rb"); // Open in binary mode
        if (file) {
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            rewind(file);

            char *content = malloc(file_size);
            fread(content, 1, file_size, file); // Read binary data

            char response_header[BUFFER_SIZE];
            snprintf(response_header, sizeof(response_header),
                     "HTTP/1.1 200 OK\nContent-Type: application/octet-stream\nContent-Length: %ld\n\n",
                     file_size);
            write(client_socket, response_header, strlen(response_header)); // Send header
            write(client_socket, content, file_size); // Send binary content

            free(content);
            fclose(file);
        } else {
            char *response = "HTTP/1.1 500 Internal Server Error\nContent-Length: 0\n\n";
            write(client_socket, response, strlen(response));
        }
    } else {
        char *response = "HTTP/1.1 404 Not Found\nContent-Length: 0\n\n";
        write(client_socket, response, strlen(response));
    }
}

// POST: Create a new file with the provided content
void post_request(int client_socket, const char* request) {
    char file_path[BUFFER_SIZE] = "./data";
    sscanf(request, "POST %s", file_path + 6);

    char *body = strstr(request, "\r\n\r\n");
    if (body) {
        body += 4; // Skip the "\r\n\r\n"
        FILE *file = fopen(file_path, "w");
        if (file) {
            fwrite(body, 1, strlen(body), file);
            fclose(file);

            char *response = "HTTP/1.1 201 Created\nContent-Length: 0\n\n";
            write(client_socket, response, strlen(response));
        } else {
            char *response = "HTTP/1.1 500 Internal Server Error\nContent-Length: 0\n\n";
            write(client_socket, response, strlen(response));
        }
    } else {
        char *response = "HTTP/1.1 400 Bad Request\nContent-Length: 0\n\n";
        write(client_socket, response, strlen(response));
    }
}

// HEAD: Return only the headers for a file
void head_request(int client_socket, const char* request) {
    char file_path[BUFFER_SIZE] = "./data";
    sscanf(request, "HEAD %s", file_path + 6);

    if (file_exists(file_path)) {
        struct stat file_stat;
        stat(file_path, &file_stat);

        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: %ld\n\n",
                 file_stat.st_size);
        write(client_socket, response, strlen(response));
    } else {
        char *response = "HTTP/1.1 404 Not Found\nContent-Length: 0\n\n";
        write(client_socket, response, strlen(response));
    }
}

// PUT: Update or create a file with the provided content
void put_request(int client_socket, const char* request) {
    char file_path[BUFFER_SIZE] = "./data";
    sscanf(request, "PUT %s", file_path + 6);

    char *body = strstr(request, "\r\n\r\n");
    if (body) {
        body += 4; // Skip the "\r\n\r\n"
        FILE *file = fopen(file_path, "w");
        if (file) {
            fwrite(body, 1, strlen(body), file);
            fclose(file);

            char *response = "HTTP/1.1 200 OK\nContent-Length: 0\n\n";
            write(client_socket, response, strlen(response));
        } else {
            char *response = "HTTP/1.1 500 Internal Server Error\nContent-Length: 0\n\n";
            write(client_socket, response, strlen(response));
        }
    } else {
        char *response = "HTTP/1.1 400 Bad Request\nContent-Length: 0\n\n";
        write(client_socket, response, strlen(response));
    }
}

// DELETE: Delete a file
void delete_request(int client_socket, const char* request) {
    char file_path[BUFFER_SIZE] = "./data";
    sscanf(request, "DELETE %s", file_path + 6);

    if (file_exists(file_path)) {
        if (remove(file_path) == 0) {
            char *response = "HTTP/1.1 200 OK\nContent-Length: 0\n\n";
            write(client_socket, response, strlen(response));
        } else {
            char *response = "HTTP/1.1 500 Internal Server Error\nContent-Length: 0\n\n";
            write(client_socket, response, strlen(response));
        }
    } else {
        char *response = "HTTP/1.1 404 Not Found\nContent-Length: 0\n\n";
        write(client_socket, response, strlen(response));
    }
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
    char response[BUFFER_SIZE];

    printf("Thread %d waiting for connections on port %d...\n", args->thread_id, args->port);

    while (1) {
        sem_wait(&thread_semaphore); // Wait for an available thread slot
        if ((new_socket = accept(args->server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            sem_post(&thread_semaphore); // Release the thread slot
            continue;
        }

        read(new_socket, buffer, BUFFER_SIZE);
        printf("Thread %d received request on port %d:\n%s\n", args->thread_id, args->port, buffer);

        switch (args->port) {
            case 21: // FTP
                snprintf(response, sizeof(response),
                         "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 24\n\nReceived FTP request");
                write(new_socket, response, strlen(response));
                break;

            case 22: // SSH
                snprintf(response, sizeof(response),
                         "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 25\n\nReceived SSH request");
                write(new_socket, response, strlen(response));
                break;

            case 23: // Telnet
                snprintf(response, sizeof(response),
                         "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 28\n\nReceived Telnet request");
                write(new_socket, response, strlen(response));
                break;

            case 25: // SMTP
                snprintf(response, sizeof(response),
                         "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 27\n\nReceived SMTP request");
                write(new_socket, response, strlen(response));
                break;

            case 53: // DNS
                snprintf(response, sizeof(response),
                         "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 26\n\nReceived DNS request");
                write(new_socket, response, strlen(response));
                break;

            case 161: // SNMP
                snprintf(response, sizeof(response),
                         "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 27\n\nReceived SNMP request");
                write(new_socket, response, strlen(response));
                break;

            case 8080: // HTTP
                handle_request(new_socket, buffer); // Delegate to handle_request()
                break;

            default:
                snprintf(response, sizeof(response),
                         "HTTP/1.1 400 Bad Request\nContent-Type: text/plain\nContent-Length: 28\n\nUnknown protocol on port.");
                write(new_socket, response, strlen(response));
                break;
        }

        close(new_socket);
        sem_post(&thread_semaphore); // Release the thread slot
    }

    return NULL;
}

//-n hils -w httproot -p puerto
int main(int argc, char *argv[]) {

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
        args[i].port = port; // Pass the port number to each thread
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