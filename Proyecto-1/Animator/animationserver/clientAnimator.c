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

pthread_mutex_t lock;

void init_screen(int heigth, int width, int client_fd){

    initscr();
    
    char s[50] = "Filas: ";
    char n[10];
    noecho();
    curs_set(FALSE);
    WINDOW *mywin = newwin(heigth,width,0,0); 
    box(mywin,0,0);
    
    int rows, cols;
    getmaxyx(mywin, rows, cols);
    sprintf(n, "%d", rows);
    strcat(s, n);
    sprintf(n, "%d", cols);
    strcat(s, "Columnas:");
    strcat(s, n);
    int len = strlen(s);
    //printw("Filas: %d, Columnas: %d\n", rows, cols);
    for (int i = 0; i < len; i++){
        mvwaddch(mywin, 0, i, s[i]);
    }
    wrefresh(mywin);
}

int process_instruction(char *input){
    int x,y;
    char c;

    char* x_ptr = strchr(input, ';')+1;
    *x_ptr = '\0';

    printf("\nread: {%s} \n",input);

    if(strcmp(input, "END;")==0){
        return -1;
    }

    sscanf(input, "PRINT:%d,%d,%c;", &y,&x,&c);
    printf("\033[%d;%dH%c",x,y,c);
    fflush(stdout);
    return 0;
}

void animation_cicle(int client_fd){

    int valread;
    
    char input_buffer[32];

    printf("\033[2J");
    char input_acumulator[64] = {0};

    //printf("\nlisten\n");
    
    while(1){
        
        valread = recv(client_fd, input_buffer, 1023,0); 
    
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
        printf("\n%s\n", input_buffer);
        input_buffer[valread] ='\0';
        strcat(input_acumulator,input_buffer);
        char *start = input_acumulator;
        char *end;

        int result;
        while((end= strchr(start, ';'))!=NULL){
            //*end = '\0';
            result= process_instruction(start);
        }

        if(result == -1){
            break;
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