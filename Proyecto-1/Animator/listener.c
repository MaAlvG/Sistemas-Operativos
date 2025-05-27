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
void draw_object_ncurses(int start_x, int start_y, int height, int width, char drawing[MAX_DRAWING_SIZE], WINDOW *win){
    int iterator=0;
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            
            if (drawing[iterator] != ' '){
                pthread_mutex_lock(&lock);
                mvwaddch(win, start_y + i, start_x + j, drawing[iterator]);
                wrefresh(win);
                pthread_mutex_unlock(&lock);
            }
            iterator++;
        }
    }
    //sleep(1);
}


void clear_object(int start_x, int start_y, int height, int width){
    
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){

            pthread_mutex_lock(&lock);
            mvaddch(start_y + i, start_x + j, ' ');
            pthread_mutex_unlock(&lock);
        }
    }

}


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
    sleep(2);
   
    

    int flag=1;
    int status, valread;
    

    printf("\nlisten\n");
    while(flag){
        char input_buffer[1024] = {0};
        valread = read(client_fd, input_buffer,
                   1024 - 1); 
    
        char* x_ptr = strchr(input_buffer, ':');
        char* end_ptr = strchr(input_buffer, ';');
        
        char instruction[10]={0};

        //printf("\nread %d\n", valread);
        if(x_ptr&&end_ptr&& x_ptr<end_ptr){
            //printf("datos validos\n");
        }else{
            //printf("datos invalidos\n");
            //flag=0;
            continue;
        }

        *x_ptr ='\0';
        *end_ptr ='\0';

        strcpy(instruction, input_buffer);
        if(strcmp(instruction, "DRAW")==0){
            char* colon_ptr = strchr(x_ptr+1, ',');
            char* left_ptr = strchr(x_ptr+1, '<');
            char* right_ptr = strchr(x_ptr+1, '>');
            *colon_ptr = '\0';
            *left_ptr = '\0';
            *right_ptr = '\0';
            int y_pos = atoi(x_ptr+1);
            int x_pos = atoi(colon_ptr+1);
            char draw[900];
            strcpy(draw, left_ptr+1);
            int draw_size = atoi(right_ptr+1);
            
            //printf("\n%d %d %s %d\n", y_pos,x_pos,draw, draw_size);

            draw_object_ncurses(x_pos, y_pos, draw_size, draw_size,draw, mywin);
        }else if(strcmp(instruction, "CLEAR")==0){
            
        }else if(strcmp(instruction, "EXPLODE")==0){
            
        }else if(strcmp(instruction, "START")==0){
            
        }else if(strcmp(instruction, "END")==0){
            flag = 0;
        }else{
            printf("instruccion no reconocida");
        }
        usleep(300000);
    }

    delwin(mywin);
    endwin();
    printf("\nstop listen\n");
    printf("\n %s",s);
}


int main(int argc, char const* argv[]){

    initscr();
    
    char s[10] ={0};
    char n[10];
    
    

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    sprintf(n, "%d", rows);
    strcat(s, n);
    strcat(s, "x");
    sprintf(n, "%d", cols);
    strcat(s, n);
    strcat(s, ";");
    int len = strlen(s);
    endwin();

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

    sleep(1);
    send(client_fd, s, strlen(s), 0);
    printf("Hello message sent\n");
    
    valread = read(client_fd, input_buffer,
                   1024 - 1); 
    
    int monitor_height=0;
    int monitor_width=0;

    char* x_ptr = strchr(input_buffer, 'x');
    char* end_ptr = strchr(input_buffer, ';');

    if(x_ptr&&end_ptr&& x_ptr<end_ptr){
        *x_ptr ='\0';
        *end_ptr ='\0';
        monitor_height = atoi(input_buffer);
        monitor_width = atoi(x_ptr+1);

        
        //printf("|%d, %d|", monitor_height, monitor_width);
    }else{
        printf("datos invalidos");
        return 0;
    }

    printf("<%s>\n", input_buffer);

    init_screen(monitor_height,monitor_width, client_fd);


    // closing the connected socket
    close(client_fd);
    return 0;
}