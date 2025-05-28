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
}


void clear_object(int start_x, int start_y, int height, int width, WINDOW *win){
    
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){

            pthread_mutex_lock(&lock);
            mvwaddch(win, start_y + i, start_x + j, ' ');
            //wrefresh(win);
            pthread_mutex_unlock(&lock);
        }
    }

}

int process_instruction(char input[1024], WINDOW *mywin){
    char* x_ptr = strchr(input, ':');
    
    char instruction[10]={0};

    //printf("\nread: {%s} \n",input);

    *x_ptr ='\0';

    strcpy(instruction, input);
    //printf("%s\n",  instruction);
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
        char* colon_ptr = strchr(x_ptr+1, ',');
        char* div_ptr = strchr(x_ptr+1, '|');

        *colon_ptr = '\0';
        *div_ptr = '\0';

        int y_pos = atoi(x_ptr+1);
        int x_pos = atoi(colon_ptr+1);

        int draw_size = atoi(div_ptr+1);
        //printf("\n%d %d %d\n", y_pos,x_pos, draw_size);
        clear_object(x_pos, y_pos, draw_size, draw_size, mywin);
    }else if(strcmp(instruction, "EXPLODE")==0){
        
    }else if(strcmp(instruction, "START")==0){
        
    }else if(strcmp(instruction, "END")==0){
        return -1;
    }else{
        printf("instruccion no reconocida");
        return 1;
    }
    return 0;
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
   
    
    int valread;
    
    char input_buffer[1024];
    char input_acumulator[2046] = {0};

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
        //printf("\n%s\n", input_buffer);
        input_buffer[valread] ='\0';
        strcat(input_acumulator,input_buffer);
        char *start = input_acumulator;
        char *end;

        int result;
        while((end= strchr(start, ';'))!=NULL){
            //*end = '\0';
            result = process_instruction(start, mywin);
            start=end+1;
            send(client_fd, "ACK", 3, 0);
        }

        if(result==-1){
            printf("\nend\n");
            break;
        }
        memmove(input_acumulator, start, strlen(start)+1);
        
        //usleep(300000);
    }
    delwin(mywin);
    endwin();
    
   
    printf("\nstop listen\n");
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

    pthread_mutex_init(&lock, NULL);
    init_screen(monitor_height,monitor_width, client_fd);


    // closing the connected socket
    close(client_fd);
    pthread_mutex_destroy(&lock);
    return 0;
}