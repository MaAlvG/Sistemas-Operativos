#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <errno.h>


#define PORT 8080
#define MAX_DRAWING_SIZE 900
#define RECV_BUFFER_SIZE 8192
#define ACCUMULATOR_SIZE 16384

void animation_cicle(int client_fd) {
    char recv_buffer[RECV_BUFFER_SIZE];
    char accumulator[ACCUMULATOR_SIZE] = {0};
    int accumulator_len = 0;
    
    // Configurar socket como no-bloqueante para mejor control
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Limpiar pantalla
    printf("\033[2J\033[H");
    fflush(stdout);
    
    fd_set read_fds;
    struct timeval timeout;
    
    while(1) {
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        
        timeout.tv_sec = 1;  // 1 segundo timeout
        timeout.tv_usec = 0;
        
        int activity = select(client_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if(activity < 0) {
            perror("select error");
            break;
        } else if(activity == 0) {
            // Timeout - continuar esperando
            continue;
        }
        
        if(FD_ISSET(client_fd, &read_fds)) {
            int valread = recv(client_fd, recv_buffer, RECV_BUFFER_SIZE - 1, 0);
            
            if(valread <= 0) {
                if(valread < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    continue; // No hay datos, continuar
                }
                printf("Conexión perdida\n");
                break;
            }
            
            recv_buffer[valread] = '\0';
            
            // Verificar espacio en accumulator
            if(accumulator_len + valread >= ACCUMULATOR_SIZE - 1) {
                printf("Buffer overflow - reseteando\n");
                accumulator_len = 0;
                accumulator[0] = '\0';
            }
            
            // Agregar datos recibidos
            strcat(accumulator, recv_buffer);
            accumulator_len += valread;
            
            // Procesar comandos completos
            char *cmd_start = accumulator;
            char *cmd_end;
            int commands_processed = 0;
            
            while((cmd_end = strchr(cmd_start, ';')) != NULL) {
                *cmd_end = '\0';
                
                int row, col;
                char c;
                
                if(sscanf(cmd_start, "PRINT:%d,%d,%c", &row, &col, &c) == 3) {
                    if(row > 0 && col > 0 && row <= 50 && col <= 200) {
                        printf("\033[%d;%dH%c", row, col, c);
                        commands_processed++;
                    }
                } else if(strncmp(cmd_start, "END", 3) == 0) {
                    printf("\nAnimación terminada\n");
                    fflush(stdout);
                    return;
                }
                
                cmd_start = cmd_end + 1;
            }
            
            // Flush después de procesar lote
            if(commands_processed > 0) {
                fflush(stdout);
            }
            
            // Mover datos restantes al inicio del buffer
            if(*cmd_start) {
                accumulator_len = strlen(cmd_start);
                memmove(accumulator, cmd_start, accumulator_len + 1);
            } else {
                accumulator_len = 0;
                accumulator[0] = '\0';
            }
        }
    }
    
    printf("\nConexión terminada\n");
}

int main(int argc, char const* argv[]){

    
    int status, valread, client_fd;
    struct sockaddr_in serv_addr;
    char* hello = "Hola servidor";
    char input_buffer[1024] = { 0 };
    
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Error de creacion del socket \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);


    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
        <= 0) {
        printf(
            "\nIp invalida/no soportada \n");
        return -1;
    }

    if ((status
         = connect(client_fd, (struct sockaddr*)&serv_addr,
                   sizeof(serv_addr)))
        < 0) {
        printf("\nConexion fallida \n");
        return -1;
    }
  
    

    //sleep(1);
    send(client_fd, hello, strlen(hello), 0);
    printf("'Hola' enviado\n");
    
    valread = recv(client_fd, input_buffer, 1023, 0); 
    
    if(valread==-1){
        printf("\nDatos recibidos invalidos\n");
        return 0;
    }

    printf("<%s>\n", input_buffer);

    //sleep(1);
    printf("\e[?25l");
    fflush(stdout);
    animation_cicle(client_fd);
    printf("\e[?25H");
    fflush(stdout);
    
    close(client_fd);
    return 0;
}
