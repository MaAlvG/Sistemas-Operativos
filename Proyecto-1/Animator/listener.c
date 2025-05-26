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

void init_screen(int heigth, int width){

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
    delwin(mywin);
    endwin();

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

        
        printf("|%d, %d|", monitor_height, monitor_width);
    }else{
        printf("datos invalidos");
        return 0;
    }

    printf("<%s>\n", input_buffer);
    init_screen(monitor_height,monitor_width);

    // closing the connected socket
    close(client_fd);
    return 0;
}