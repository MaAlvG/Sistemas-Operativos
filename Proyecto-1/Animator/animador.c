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
#include <ncurses.h>


#define MAX_OBJECT_WIDTH 20
#define MAX_OBJECT_HEIGT 20

#define MAX_OBJECT_SIZE 20
// typedef struct {
//     int width;
//     int height;
//     char **grid; // Matriz 2D de caracteres ASCII
//     my_mutex_t **locks; // Para control concurrente de cada celda
// } Canvas;

typedef struct{
    int width;
    int height;
    char drawing[MAX_OBJECT_HEIGT][MAX_OBJECT_WIDTH];
} Object;


int load_shape_from_file(Object *obj, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error abriendo archivo");
        return -1;
    }

    char line[MAX_OBJECT_SIZE + 2];  // +2 para '\n' y '\0'
    int row = 0;

    while (fgets(line, sizeof(line), fp) && row < MAX_OBJECT_SIZE) {
        int len = strlen(line);

        // Quitar salto de línea
        if (line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }

        if (len > obj->width) obj->width = len;
        
        printf("\n");
        for(int i = 0; i<len;i++){
            
            printf("%d ", line[i]);
            if(line[i]== NULL){
                line[i] = ' ';
            }
        }
        for (int col = 0; col < len && col < MAX_OBJECT_SIZE; col++) {
            obj->drawing[row][col] = line[col];
        }
        row++;
    }

    obj->height = row;

    fclose(fp);
    return 0;
}

void draw_object_ncurses(Object *obj, int start_x, int start_y) {
    for (int i = 0; i < obj->height; i++) {
        for (int j = 0; j < obj->width; j++) {
            char ch = obj->drawing[i][j];
            printf("%d",ch);
            if (ch != ' ')
                mvaddch(start_y + i, start_x + j, ch);
        }
    }
}


int main(int argc, char *argv[]) {
    // initscr();              // Inicia ncurses
    // cbreak();               // Desactiva el buffer de línea
    // noecho();               // No muestra las teclas presionadas
    // curs_set(0); 
    //            // Oculta el cursor
    // // loadObject();
    // // mvprintw(10, 20, "Hola desde ncurses!"); // Imprime en (fila 10, col 20)
    // refresh();              // Refresca la pantalla para mostrar los cambios

    // getch();                // Espera una tecla
    // endwin();  
    // loadObject();             // Termina ncurses y restaura la terminal

    Object obj = {0};

    if (load_shape_from_file(&obj, "ascii-object.txt") != 0)
        return 1;

    initscr();
    noecho();
    curs_set(FALSE);

    draw_object_ncurses(&obj, 1, 1);

    refresh();
    getch();
    endwin();

    return 0;
}
