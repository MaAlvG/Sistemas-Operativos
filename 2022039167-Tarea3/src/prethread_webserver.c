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
} thread_args;

sem_t thread_semaphore; // Semaphore to track available threads
char root_directory[BUFFER_SIZE] = "./data"; // Default root directory

int file_exists(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

const char* get_content_type(const char* file_path) {
    const char *ext = strrchr(file_path, '.'); // Buscar la extensión
    if (ext == NULL) return "application/octet-stream"; // Tipo predeterminado

    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".pdf") == 0) return "application/pdf";

    // Otros tipos de archivo
    return "application/octet-stream"; // Tipo genérico binario
}

// GET: Read and return the content of a file
void get_request(int client_socket, const char* request) {
    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s", root_directory); // Use root_directory
    sscanf(request, "GET %s ", file_path + strlen(root_directory));

    printf("\np1%s\n", file_path);
    if (file_exists(file_path)) {
        printf("\np2%s\n", file_path);
        FILE *file = fopen(file_path, "rb"); // Open in binary mode
        if (file) {
            fseek(file, 0, SEEK_END);
            long file_size = ftell(file);
            rewind(file);

            char *content = malloc(file_size);
            fread(content, 1, file_size, file);

            // char response[BUFFER_SIZE];
            // snprintf(response, sizeof(response),
            //          "HTTP/1.1 200 OK\nContent-Type: application/octet-stream\nContent-Length: %ld\n\n",
            //          file_size);
            // write(client_socket, response, strlen(response));
            // write(client_socket, content, file_size); // Send binary content

            const char *content_type = get_content_type(file_path); // Detectar tipo de archivo
            char response_header[BUFFER_SIZE];
            
            snprintf(response_header, sizeof(response_header),
                     "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %ld\n\n",
                     content_type, file_size);

            write(client_socket, response_header, strlen(response_header)); // Enviar encabezado
            write(client_socket, content, file_size);

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
    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s", root_directory); // Use root_directory
    sscanf(request, "HEAD %s", file_path + strlen(root_directory));
    
    if (file_exists(file_path)) {
        printf("\npath %s\n", file_path);
        struct stat file_stat;
        if (stat(file_path, &file_stat) != 0) {
            // char response[BUFFER_SIZE];
            // snprintf(response, sizeof(response),
            //          "HTTP/1.1 200 OK\r\n"
            //          "Content-Type: application/octet-stream\r\n"
            //          "Content-Length: %ld\r\n\r\n", // Ensure proper HTTP header formatting
            //          file_stat.st_size);
            // write(client_socket, response, strlen(response)); // Send only headers

            const char *content_type = get_content_type(file_path); // Detectar tipo de archivo
            char response_header[BUFFER_SIZE];
            
            snprintf(response_header, sizeof(response_header),
                        "HTTP/1.1 200 OK\nContent-Type: %s\nContent-Length: %ld\n\n",
                        content_type, file_stat.st_size);

            write(client_socket, response_header, strlen(response_header));

        } else {
            char *response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
            write(client_socket, response, strlen(response)); // Send error headers
        }
    } else {
        char *response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        write(client_socket, response, strlen(response)); // Send error headers
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
    int arg_opt;
    int num_threads = 10;
    int port = DEFAULT_PORT;
    extern char *optarg;
    extern int optind;

    while ((arg_opt = getopt(argc, argv, "n:w:p:")) != -1) {
        switch (arg_opt) {
            case 'n':
                num_threads = atoi(optarg);
                break;
            case 'w':
                snprintf(root_directory, sizeof(root_directory), "%s", optarg); // Update root_directory
                break;
            case 'p':
                port = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Uso: %s [-n hilos] [-w direccion] [-p puerto]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    fprintf(stderr, "Uso: %s [-n hilos] [-w direccion] [-p puerto]\n", argv[0]);

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