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


#define PORT 8080
#define MAX_DRAWING_SIZE 900

void animation_cicle(int client_fd){

    int valread;
    
    char input_buffer[64];
    char input_acumulator[128] = {0};
    printf("\033[2J");
    
    int flag =1;
    while(1){
        
        valread = recv(client_fd, input_buffer, sizeof(input_buffer)-1,0); 
    
        if(valread<=0){
            if(valread<0){
                perror("error al leer");
            }
            break;
        }
        input_buffer[valread] ='\0';
        

        strcat(input_acumulator,input_buffer);
        char *start = input_acumulator;
        char *end;

        while((end= strchr(start, ';'))!=NULL){
            *end = '\0';
            int x,y;
            char c;

            if(sscanf(start, "PRINT:%d,%d,%c", &y,&x,&c)==3){
                printf("\033[%d;%dH%c",y,x,c);
                fflush(stdout);
            }
            start = end+1;
        }

        if(*start){
            memmove(input_acumulator, start, strlen(start)+1);
        }else{
            memset(input_acumulator,0,sizeof(input_acumulator));
        }

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
    printf("\e[?25l");
    fflush(stdout);
    animation_cicle(client_fd);
    printf("\e[?25H");
    fflush(stdout);
    // closing the connected socket
    close(client_fd);
    return 0;
}