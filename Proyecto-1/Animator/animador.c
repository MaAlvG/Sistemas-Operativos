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


#define MAX_OBJECT_SIZE 30

int canvas_width = 0;
int canvas_heigth = 0;

typedef struct 
{
    int width;
    int height;
    pthread_mutex_t **locks; // Para control concurrente de cada celda
} Canvas;

pthread_mutex_t lock;

//Object

typedef struct{
    int id;
    int socket;
    int height;
    int width;
} Monitor;

// typedef struct{
//     int width;
//     int height;
//     char drawing[MAX_OBJECT_SIZE][MAX_OBJECT_SIZE];
// } Explosion_obj;

typedef struct{
    char* id;
    int x, y;                                       // Posición actual
    int width, height;                              // Tamaño
    char drawing[MAX_OBJECT_SIZE][MAX_OBJECT_SIZE];// Dibujo (matriz de ASCII)
    int rotations;                                      // Rotación:
    // SchedulerType scheduler;
    int start_time, end_time; // Tiempo de aparición y desaparición
    int destined_x, destined_y; // Movimiento por frame
    int active; 
    int movements ;             // Si está activo o esperando
} Object;

typedef struct{
    Object* objeto;
    //Monitor** monitor;
    Canvas* canvas;
} thread_args;

// int load_shape_from_file(Object *obj, const char *filename){
//     FILE *fp = fopen(filename, "r");
//     if (!fp){
//         perror("Error abriendo archivo");
//         return -1;
//     }
//
//     char line[MAX_OBJECT_SIZE + 2]; // +2 para '\n' y '\0'
//     int row = 0;
//
//     while (fgets(line, sizeof(line), fp) && row < MAX_OBJECT_SIZE){
//         int len = strlen(line);
//
//         // Quitar salto de línea
//         if (line[len - 1] == '\n'){
//             line[len - 1] = '\0';
//             len--;
//         }
//
//         if (len > obj->width)
//             obj->width = len;
//
//         for (int col = 0; col < len && col < MAX_OBJECT_SIZE; col++){
//             obj->drawing[row][col] = line[col];
//         }
//         row++;
//     }
//
//     obj->height = row;
//
//     fclose(fp);
//     return 0;
// }

/*identificador, x de inicio, y de inicio, x final, y final, tiempo de inicio, tiempo de final,*/
Object *init_object(char* id, int x, int y, int dx, int dy, int sT, int eT, int rot){
    Object *obj = calloc(1, sizeof(Object));
    obj->id = id;
    obj->x = x;
    obj->y = y;
    obj->start_time = sT;
    obj->end_time = eT;
    // obj->scheduler = sched;
    obj->rotations = rot;
    obj->destined_x = dx;
    obj->destined_y = dy;
    obj->active = 1;
    return obj;
}

/*objeto, x de inicio, x de final*/
void draw_object_ncurses(Object *obj){
    int start_x = obj->x;
    int start_y = obj->y;

    for (int i = 0; i < obj->height; i++){
        for (int j = 0; j < obj->width; j++){
            
            if (obj->drawing[i][j] != ' '){
                pthread_mutex_lock(&lock);
                mvaddch(start_y + i, start_x + j, obj->drawing[i][j]);
                refresh();
                pthread_mutex_unlock(&lock);
            }
            
        }
    }
}

void update_locks(int step_x,int step_y,Object *obj, Canvas* canvas){
    int start_x = obj->x;
    int start_y = obj->y;
    for (int i = 0; i < obj->height; i++){
        for (int j = 0; j < obj->width; j++){

            int flag_y = i+step_y < 0||i+step_y > obj->height-1;
            int flag_x = j+step_x < 0||j+step_x > obj->width-1;
            

            if(flag_x&&!flag_y){
                pthread_mutex_lock(&canvas->locks[start_y + i][start_x + j+step_x]);

            }else if(!flag_x&&flag_y){
                pthread_mutex_lock(&canvas->locks[start_y + i+step_y][start_x + j]);

            }else if(flag_x&&flag_y){
                pthread_mutex_lock(&canvas->locks[start_y + i+step_y][start_x + j + step_x]);

            }
            
        }
    }

    int new_start_x = start_x+step_x;
    int new_start_y = start_y+step_y;

    step_x= step_x*-1;
    step_y= step_y*-1;

    for (int i = 0; i < obj->height; i++){
        for (int j = 0; j < obj->width; j++){

            
            int flag_y = i+step_y < 0||i+step_y > obj->height-1;
            int flag_x = j+step_x < 0||j+step_x > obj->width-1;
            

            if(flag_x&&!flag_y){
                pthread_mutex_unlock(&canvas->locks[new_start_y + i][new_start_x + j+step_x]);

            }else if(!flag_x&&flag_y){
                pthread_mutex_unlock(&canvas->locks[new_start_y + i+step_y][new_start_x + j]);

            }else if(flag_x&&flag_y){
                pthread_mutex_unlock(&canvas->locks[new_start_y + i+step_y][new_start_x + j + step_x]);

            }
        }
    }
}

void release_locks(Object *obj, Canvas* canvas){
    int start_x = obj->x;
    int start_y = obj->y;
    for (int i = 0; i < obj->height; i++){
        for (int j = 0; j < obj->width; j++){
            pthread_mutex_unlock(&canvas->locks[start_y + i][start_x + j]);
        }
    }
}

int rotate(Object* obj, int new_angle){

    int rotations = new_angle/90;
    //printf("\n%d, %d, %d\n",obj->height,obj->width, rotations);

    // while(rotations){
    //     char new_drawing[obj->width][obj->height];
    //     int old_width = obj->width;
    //     int old_height = obj->height;

    //     int x = old_height-1;
    //     for(int i = 0; i < old_height;i++){
    //         for(int j = 0; j < old_width; j++){
                
    //             new_drawing[j][x] = obj->drawing[i][j];
    //             //obj->drawing[i][j] = 0;
                
    //             printf("i%d, j%d, x%d: %c|",i,j,x,new_drawing[j][x]);
    //         }
    //         x--;
    //         printf("\n");
    //     }

    //     for(int i = 0; i < old_height;i++){
    //         for(int j = 0; j < old_width; j++){
                
    //             obj->drawing[i][j] = new_drawing[i][j];
    //         }
    //     }

    //     obj->height = old_width;
    //     obj->width = old_height;
    //     rotations--;
    // }
    // printf("\n%d, %d, %d\n",obj->height,obj->width, rotations);

    while(rotations){
        char new_drawing[obj->width][obj->height];
        int row = 0;
        int old_width = obj->width;
        int old_height = obj->height;

        int x = old_height-1;

        for(int i = 0; i < old_height;i++){
            for(int j = 0; j < old_width; j++){
                
                new_drawing[j][x] = obj->drawing[i][j];
                //printf("%c",new_drawing[j][x]);
                obj->drawing[i][j] = ' ';
            }
            //printf("  %d\n", x);
            x--;
        }

        for(int i = 0; i < old_height;i++){
            for(int j = 0; j < old_width; j++){
                
                obj->drawing[i][j] = new_drawing[i][j];
            }
        }

        rotations--;
    }

    return 0;
}

void explode(Object* obj){
    return;
}

void clear_object(Object* obj){
    int start_x = obj->x;
    int start_y = obj->y;
    for (int i = 0; i < obj->height; i++){
        for (int j = 0; j < obj->width; j++){

            pthread_mutex_lock(&lock);
            mvaddch(start_y + i, start_x + j, ' ');
            pthread_mutex_unlock(&lock);
        }
    }

}

void move_object(Object* obj, Canvas* canvas){
    int move_flag  = 1;

    while(move_flag){
        
        draw_object_ncurses(obj);
        
        int step_x = 0;
        int step_y = 0;

        if(obj->destined_x > obj->x){
            step_x = 1;
        }else if(obj->destined_x < obj->x){
            step_x = -1;
        }

        if(obj->destined_y > obj->y){
            step_y = 1;
        }else if(obj->destined_y < obj->y){
            step_y = -1;
        }
        
        

        update_locks(step_x,step_y,obj, canvas);
        

        int new_x, new_y;
        if(obj->destined_x != obj->x){
            new_x = obj->x + step_x;
        }

        if(obj->destined_y != obj->y){
            new_y = obj->y + step_y;
        }
        
        //update(obj, obj->x, obj->y, new_x, new_y, canvas);
        
        rotate(obj, obj->rotations);

        usleep(300000);

        
        //rotate(obj, obj->rotations);
        clear_object(obj);
        
        //refresh();
        //usleep(300000);

        obj->x = new_x;
        obj->y = new_y;
        move_flag = obj->destined_x != obj->x || obj->destined_y != obj->y;
        
    }
    release_locks(obj, canvas);
    // sleep(1);
    // clear();
    // explode(obj);

}

int load_config(const char* filename, Canvas* canvas, Object  ***arr, int *size){
    FILE *fp = fopen(filename, "r");
    if (!fp){
        perror("Error abriendo archivo");
        return -1;
    }

    char line[30];
    fgets(line, 30, fp);
    

    fgets(line, 10, fp);
    canvas->height  = atoi(line);

    fgets(line, 10, fp);
    canvas->width  = atoi(line);


    canvas->locks = malloc(canvas->height * sizeof(pthread_mutex_t*));

    for(int i =0;i<canvas->height; i++){
        canvas->locks[i] = malloc(canvas->width * sizeof(pthread_mutex_t));

        for(int j =0;j<canvas->width; j++){
            pthread_mutex_init(&canvas->locks[i][j], NULL);
        }
    }

    int num_objects;
    fgets(line, 2, fp);
    fgets(line, 14, fp);
    fgets(line, 5, fp);
    
    num_objects = atoi(line);

    for(int i =0; i< num_objects; i++){
        char obj_id[20];
        char scheduler[15];
        int start_time;
        int end_time;
        int start_x, start_y;
        int end_x, end_y;
        int rotation;

        fgets(line, 2, fp);
        fgets(line, 27, fp);

        fgets(line, 14, fp);
        fgets(line, 30, fp);
        int len = strlen(line);
        if (line[len - 1] == '\n'){
            line[len - 1] = '\0';
            len--;
        }
        strcpy(obj_id,line);

        fgets(line, 2, fp);
        fgets(line, 14, fp);
        fgets(line, 30, fp);
        len = strlen(line);
        if (line[len - 1] == '\n'){
            line[len - 1] = '\0';
            len--;
        }
        strcpy(scheduler,line);

        fgets(line, 2, fp);
        fgets(line, 14, fp);
        fgets(line, 5, fp);
        
        start_time = atoi(line);

        fgets(line, 2, fp);
        fgets(line, 14, fp);
        fgets(line, 5, fp);
        
        end_time = atoi(line);

        fgets(line, 2, fp);
        fgets(line, 14, fp);

        fgets(line, 10, fp);
        start_x  = atoi(line);

        fgets(line, 10, fp);
        start_y  = atoi(line);

        fgets(line, 2, fp);
        fgets(line, 14, fp);
        fgets(line, 10, fp);
        end_x  = atoi(line);

        fgets(line, 10, fp);
        end_y  = atoi(line);
        fgets(line, 2, fp);
        fgets(line, 14, fp);
        fgets(line, 5, fp);
        
        rotation = atoi(line);

        Object* obj = init_object(obj_id, start_x, start_y, end_x, end_y, start_time, end_time, rotation);

        fgets(line, 2, fp);
        fgets(line, 14, fp);

        fgets(line, 5, fp);
        int row=0;
        int shape_size= atoi(line);

        int flag=0;
        fgets(line, 25, fp);
        for(int j=0; j<shape_size;j++){
            int len = strlen(line);
            //printf("\n{%d}\n", j);
            // Quitar salto de línea
            if (line[len - 1] == '\n'){
                line[len - 1] = '\0';
                len--;
            }
            //printf("\n{{%d}\n", j);
            if (len > obj->width)
                obj->width = len;
            //printf("\n{%d}}\n", j);
            for (int col = 0; col < len && col < MAX_OBJECT_SIZE; col++){
                obj->drawing[row][col] = line[col];
            }
            row++;
            fgets(line, 25, fp);
        }

        obj->height = row;
            
        
        
        *arr = realloc(*arr, (*size+1)*sizeof(Object *));
        (*arr)[*size] = obj;
        (*size)++;
        fgets(line, 25, fp);

        //printf("%d|%d\n", end_x, end_y);
    }
    fclose(fp);
    return 0;
}

void* handle_animation(void* arg){
    thread_args* args =(thread_args* )arg;
    move_object(args->objeto, args->canvas);

    //printf("animando \n");

    return NULL;
}

int main(int argc, char *argv[]){

    int arg_opt = 0;
    extern char *optarg;
    char *config_file = NULL;

    while((arg_opt = getopt(argc,argv, "c:"))!=-1){
        switch (arg_opt)
        {
        case 'c':
            config_file =optarg;
            break;
        
        default:
            printf("\nArchivo de configuracion no especificado\n");
            exit(EXIT_FAILURE);
        }
    }

    Canvas* canvas = calloc(1, sizeof(Canvas));

    //Object *obj = init_object("test", 0, 0,5, 5, 10, 10);
    //Object* obj =calloc(1, sizeof(Object));

    Object** obj_list = NULL;
    int num_objects=0;
    load_config(config_file, canvas, &obj_list, &num_objects);

    pthread_t threads[num_objects];
    thread_args args[num_objects];

    // for(int i=0;i<num_objects;i++){
        
    //     printf("\n>%d x:%d y:%d\n",i, obj_list[i]->destined_x,obj_list[i]->destined_y);
    // }

    initscr();
    noecho();
    curs_set(FALSE);  

    pthread_mutex_init(&lock, NULL);

    for(int i=0;i<num_objects;i++){
        args[i].canvas = canvas;
        args[i].objeto = obj_list[i];
        
        pthread_create(&threads[i], NULL, handle_animation, &args[i]);
    }
    
    

    // int rows, cols;
    // getmaxyx(stdscr, rows, cols);
    // printf("Filas: %d, Columnas: %d\n", rows, cols);

    // Wait for all threads to complete
    for (int i = 0; i < num_objects; i++) {
        pthread_join(threads[i], NULL);
    }

    sleep(1);
    //refresh();
    endwin();

    for(int i =0;i<canvas->height; i++){
        for(int j =0;j<canvas->width; j++){
            pthread_mutex_destroy(&canvas->locks[i][j]);
        }
    }

    pthread_mutex_destroy(&lock);
    return 0;
}
