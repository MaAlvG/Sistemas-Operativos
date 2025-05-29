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
#include <ncurses.h>


#define PORT 8080
#define MAX_DRAWING_SIZE 900

void process_instruction(char *input){
    int x,y;
    char c;

    sscanf(input, "PRINT:%d,%d,%c;", &y,&x,&c);
    printf("\033[%d;%dH%c",x,y,c);
    fflush(stdout);
}

void animation_cicle(int client_fd){

    int valread;
    
    char input_buffer[32];

    printf("\033[2J");
    char input_acumulator[64] = {0};

    //printf("\nlisten\n");
    
    while(1){
        
        valread = recv(client_fd, input_buffer, sizeof(input_buffer)-1,0); 
    
        //printf("%d", valread);
        if(valread<=0){
            if(valread==0){
                printf("cierre del servidor\n");
            //flag=0;
            
            }else{
                perror("error al leer");
            }
            break;
        }
        //printf("\n%s\n", input_buffer);
        input_buffer[valread] ='\0';
        strcat(input_acumulator,input_buffer);
        char *start = input_acumulator;
        char *end;

        if(strcmp(start,"END;") == 0){
            break;
        }
        while((end= strchr(start, ';'))!=NULL){
            *end = '\0';
            process_instruction(start);
            start = end+1;
        }

        
        memmove(input_acumulator, start, strlen(start)+1);
    }

    printf("\nEND\n");
}
int main(int argc, char const* argv[]){

    int status, valread, client_fd;
    struct sockaddr_in serv_addr;
    char* hello = "Hello from client";
    char input_buffer[1024] = { 0 };
    
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, "192.168.100.26", &serv_addr.sin_addr)
        <= 0) {
        printf(
            "\nInvalid address/ Address not supported \n");
        return -1;
    }

    if ((status
         = connect(client_fd, (struct sockaddr*)&serv_addr,
                   sizeof(serv_addr)))
        < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
  
    
    // subtract 1 for the null
    // terminator at the end

    //sleep(1);
    send(client_fd, hello, strlen(hello), 0);
    printf("Hello message sent\n");
    
    valread = recv(client_fd, input_buffer, 1023, 0); 
    
    if(valread==-1){
        printf("\ndatos invalidos\n");
        return 0;
    }

    printf("<%s>\n", input_buffer);

    //init_screen(monitor_height,monitor_width, client_fd);
    //sleep(1);
    animation_cicle(client_fd);
    // closing the connected socket
    close(client_fd);
    return 0;
}