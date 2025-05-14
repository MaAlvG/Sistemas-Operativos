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

#define MAX_CANVAS_WIDTH 20
#define MAX_CANVAS_HEIGT 20

#define MAX_OBJECT_SIZE 20
// typedef struct 
// {
//     int width;
//     int height;
//     char **grid; // Matriz 2D de caracteres ASCII
//     my_mutex_t **locks; // Para control concurrente de cada celda
// } Canvas;

typedef struct
{
    int width;
    int height;
    char drawing[MAX_OBJECT_SIZE][MAX_OBJECT_SIZE];
} Objeto;

typedef struct
{
    int id;
    int x, y;                                       // Posición actual
    int width, height;                              // Tamaño
    char drawing[MAX_OBJECT_SIZE][MAX_OBJECT_SIZE]; // Forma (matriz de ASCII)
    int angle;                                      // Rotación: 0, 90, 180, 270
    // SchedulerType scheduler;
    int start_time, end_time; // Tiempo de aparición y desaparición
    int destined_x, destined_y; // Movimiento por frame
    int active; 
    int movements ;             // Si está activo o esperando
} Object;

int load_shape_from_file(Object *obj, const char *filename){
    FILE *fp = fopen(filename, "r");
    if (!fp){
        perror("Error abriendo archivo");
        return -1;
    }

    char line[MAX_OBJECT_SIZE + 2]; // +2 para '\n' y '\0'
    int row = 0;

    while (fgets(line, sizeof(line), fp) && row < MAX_OBJECT_SIZE){
        int len = strlen(line);

        // Quitar salto de línea
        if (line[len - 1] == '\n'){
            line[len - 1] = '\0';
            len--;
        }

        if (len > obj->width)
            obj->width = len;

        for (int col = 0; col < len && col < MAX_OBJECT_SIZE; col++){
            obj->drawing[row][col] = line[col];
        }
        row++;
    }

    obj->height = row;

    fclose(fp);
    return 0;
}

/*objeto, identificador, x de inicio, y de inicio, x final, y final, tiempo de inicio, tiempo de final,*/
Object *init_object(int id, int x, int y, int dx, int dy, int sT, int eT)
{
    Object *obj = calloc(1, sizeof(Object));
    obj->id = id;
    obj->x = x;
    obj->y = y;
    obj->start_time = sT;
    obj->end_time = eT;
    // obj->scheduler = sched;
    obj->angle = 0;
    obj->destined_x = dx;
    obj->destined_y = dy;
    obj->active = 1;

    if (load_shape_from_file(obj, "ascii-object.txt") != 0){
        return NULL;
    }

    return obj;
}

void draw_object_ncurses(Object *obj, int start_x, int start_y){
    for (int i = 0; i < obj->height; i++){
        for (int j = 0; j < obj->width; j++){
            char ch = obj->drawing[i][j];
            if (ch != ' ')
                mvaddch(start_y + i, start_x + j, ch);
        }
    }
}

int rotate_left(Object* obj){

    char new_drawing[obj->width][obj->height];
    int row = 0;
    int old_width = obj->width;
    int old_height = obj->height;

    for(int i = 0; i < old_width;i++){
        for(int j = 0; j < old_height; j++){
            
        }
    }

    while (fgets(line, sizeof(line), fp) && row < MAX_OBJECT_SIZE){
        int len = strlen(line);

        // Quitar salto de línea
        if (line[len - 1] == '\n'){
            line[len - 1] = '\0';
            len--;
        }

        if (len > obj->width)
            obj->width = len;
            
        for (int col = 0; col < len && col < MAX_OBJECT_SIZE; col++){
            obj->drawing[row][col] = line[col];
        }
        row++;
    }

    obj->height = row;
}

int rotate_right(char drawing[][]){
    return;
}

int rotate_top(char drawing[][]){
    return;
}


int rotate_object(Object* obj,int new_angle){
    if(obj->angle == new_angle)
        return 0;
    

    int old_angle = obj->angle ;
    if(new_angle == 0 || new_angle==180){

        return rotate_top(obj->drawing);

    }else if(new_angle== 90){
        return rotate_right(obj->drawing);

    }else if(new_angle == 270){
        return rotate_left(obj);

    }else{
        prtinf("angulo invalido");
        return 1;
    }
}

// void move_object(Object* obj){
//     if()
// }

int main(int argc, char *argv[])
{
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

    Object *obj = init_object(1, 0, 0, 5, 5, 10, 10);
    // if (load_shape_from_file(obj, "ascii-object.txt") != 0)
    //     return 1;
    // while(obj->active)

    initscr();
    noecho();
    curs_set(FALSE);

    start_color(); /* Start color 			*/
    init_pair(1, COLOR_RED, COLOR_BLACK);

    attron(COLOR_PAIR(1));
    draw_object_ncurses(obj, 0, 0);
    attroff(COLOR_PAIR(1));
    refresh();

    getch();
    endwin();
    rotate_object(obj, 90);
    return 0;
}
