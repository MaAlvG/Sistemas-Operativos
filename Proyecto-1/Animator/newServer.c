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
#define MAX_OBJECT_SIZE 30
#define MAX_MONITORS 4
#define MAX_MONITOR_HEIGHT 31;
#define MAX_MONITOR_WIDTH 128;

pthread_mutex_t conection_mutex;
pthread_mutex_t lock;
int counter=0;
int saved_monitors=0;

typedef struct{
    int id;
    int socket;
    int height;
    int width;
    pthread_mutex_t monitor_lock;
} Monitor;

typedef struct {
    int width;
    int height;
    pthread_mutex_t **locks; 
    int monitors_width;
    int monitors_height;
    Monitor *monitors[MAX_MONITORS][MAX_MONITORS];
    int amount_monitors;
} Canvas;

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
} animation_thread_args;

typedef struct{
    int *socket;
    Canvas* canvas;
} monitors_thread_args;
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

Monitor* new_monitor(int id,int socket, int height, int width){
    Monitor* monitor = calloc(1,sizeof(Monitor));

    monitor->id = id;
    monitor->socket= socket;
    int max_height = MAX_MONITOR_HEIGHT;
    if(max_height< height){
        monitor->height=MAX_MONITOR_HEIGHT;
    }else{
        monitor->height=height;
    }

    int max_width = MAX_MONITOR_WIDTH;
    if(max_width< width){
        monitor->width =MAX_MONITOR_WIDTH;
    }else{
        monitor->width=width;
    }

    return monitor;
}

int load_config(const char* filename, Canvas* canvas, Object  ***arr, int *size){
    FILE *fp = fopen(filename, "r");
    if (!fp){
        perror("Error abriendo archivo");
        return -1;
    }

    //Define propiedades del canvas
    char line[30];
    fgets(line, 30, fp);
    //ancho
    fgets(line, 10, fp);
    canvas->height  = atoi(line);
    //alto
    fgets(line, 10, fp);
    canvas->width  = atoi(line);

    canvas->locks = malloc(canvas->height * sizeof(pthread_mutex_t*));

    //inicia el arreglo de mutex del canvas
    for(int i =0;i<canvas->height; i++){
        canvas->locks[i] = malloc(canvas->width * sizeof(pthread_mutex_t));

        for(int j =0;j<canvas->width; j++){
            pthread_mutex_init(&canvas->locks[i][j], NULL);
        }
    }
    
    //Disposicion de los monitores
    int amount_monitors,heigth_monitors, width_monitors;
    fgets(line, 2, fp);
    fgets(line, 22, fp);
    fgets(line, 22, fp);
    fgets(line, 14, fp);
    amount_monitors = atoi(line);
    canvas->amount_monitors = amount_monitors;
    fgets(line, 14, fp);
    heigth_monitors = atoi(line);
    canvas->monitors_height = heigth_monitors;
    fgets(line, 14, fp);
    width_monitors = atoi(line);
    canvas->monitors_width = width_monitors;

    //Cantidad de objetos para la animacion
    int num_objects;
    fgets(line, 2, fp);
    fgets(line, 14, fp);
    fgets(line, 5, fp);
    
    num_objects = atoi(line);
    //ciclo para obtener la informacion de cada objeto
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

void add_monitor(Monitor *monitor,Canvas *canvas){
    int y_monitors= canvas->monitors_height;
    int x_monitors = canvas->monitors_width;
    
    printf("\n{%d %d}\n",y_monitors, x_monitors);
    for(int i =0; i<y_monitors;i++){
        for(int j=0; j<x_monitors;j++){
            if(canvas->monitors[i][j]== NULL){
                canvas->monitors[i][j]= monitor;
                i= y_monitors+1;
                j=x_monitors+1;
                //printf("\n{%d %d}\n",monitor->height, monitor->width);
            }else{
                //printf("no era null");
            }
        }   
    }

    //printf("|%d %d|", canvas->monitors[0][0]->id,canvas->monitors[0][0]->socket);
}

void draw_object_ncurses(Object *obj, Canvas *canvas){

    int start_x = obj->x;
    int start_y = obj->y;

    int monitor_x = MAX_MONITOR_HEIGHT;
    int monitor_y = MAX_MONITOR_WIDTH;
    
    int monitor_start_x= start_x;
    int monitor_start_y= start_y;

    if(start_x> monitor_x){
        monitor_start_x= start_x-MAX_MONITOR_WIDTH; 
    }

    if(start_y> monitor_y){
        monitor_start_y= start_y-MAX_MONITOR_WIDTH; 
    }
    
    //printf("\n%d %d\n", monitor_x, monitor_y);
    monitor_x= start_x/monitor_x;
    monitor_y= start_y/monitor_y;
    
    //printf("\n%d %d\n", monitor_x, monitor_y);
    int obj_len = obj->height* obj->width;
    char obj_figure[obj_len];
    int iterator=0;
    for(int i =0; i< obj->height;i++){
        for(int j =0; j< obj->width;j++){
        obj_figure[iterator]= obj->drawing[i][j];
        iterator++;
        }
    }
    obj_figure[iterator]= '\0';

    char send_draw[1024] ={0};

    sprintf(send_draw, "DRAW:%d,%d<%s>%d;",monitor_start_x,monitor_start_y,obj_figure, obj->height);
    // char n[20]={0};

    // strcat(send_draw, "DRAW:");
    // sprintf(n, "%d",monitor_start_x);
    // strcat(send_draw,n);
    // strcat(send_draw,",");
    // sprintf(n, "%d",monitor_start_y);
    // strcat(send_draw,n);
    // strcat(send_draw,">");
    // strcat(send_draw, obj_figure);
    // strcat(send_draw, ";");
    Monitor *monitor = canvas->monitors[monitor_y][monitor_x];
    if(monitor != NULL){
        printf("[\n%s\n", send_draw);
    
        printf("\n%d\n", monitor->id);
        printf("\n%d\n]", monitor->socket);
        int sev_val= send(monitor->socket, send_draw, sizeof(send_draw),0);
        printf("Respuesta enviada%d\n", sev_val);
    }else{
        printf("nulo");
        
    }

    
    
}

void clear_object(Object* obj){
    int start_x = obj->x;
    int start_y = obj->y;

    int monitor_x = MAX_MONITOR_HEIGHT;
    int monitor_y = MAX_MONITOR_WIDTH;
    
    int monitor_start_x= start_x;
    int monitor_start_y= start_y;

    if(start_x> monitor_x){
        monitor_start_x= start_x-MAX_MONITOR_WIDTH; 
    }

    if(start_y> monitor_y){
        monitor_start_y= start_y-MAX_MONITOR_WIDTH; 
    }
    
    monitor_x= start_x/monitor_x;
    monitor_y= start_y/monitor_y;

    // for (int i = 0; i < obj->height; i++){
    //     for (int j = 0; j < obj->width; j++){

    //         pthread_mutex_lock(&lock);
    //         mvaddch(start_y + i, start_x + j, ' ');
    //         pthread_mutex_unlock(&lock);
    //     }
    // }
    char send_clear[100]={0};
    sprintf(send_clear, "CLEAR:%d,%d;",monitor_start_x,monitor_start_y);
    // char n[20]={0};
    // strcat(send_clear, "CLEAR:");
    // sprintf(n, "%d",monitor_start_x);
    // strcat(send_clear,n);
    // strcat(send_clear,",");
    // sprintf(n, "%d",monitor_start_y);
    // strcat(send_clear,n);
    printf("\n%s\n", send_clear);

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

void move_object(Object* obj, Canvas* canvas){
    int move_flag  = 1;
    while(move_flag){
        
        draw_object_ncurses(obj, canvas);
        
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

void* handle_monitors(void* arg) {
    counter++;
    monitors_thread_args* args =(monitors_thread_args* )arg;
    int client_socket = *args->socket;
    Canvas *canvas = args->canvas;

    char input_buffer[1024] = {0};


    int read_input = read(client_socket, input_buffer, 1024);
    int monitor_height=0;
    int monitor_width=0;
    if(read_input==-1){
        printf("conexion invalida");
        return NULL;
    }
    char* x_ptr = strchr(input_buffer, 'x');
    char* end_ptr = strchr(input_buffer, ';');

    if(x_ptr&&end_ptr&& x_ptr<end_ptr){
        *x_ptr ='\0';
        *end_ptr ='\0';
        monitor_height = atoi(input_buffer);
        monitor_width = atoi(x_ptr+1);

        
    }else{
        printf("datos invalidos");
        return NULL;
    }
    Monitor *monitor = new_monitor(counter, client_socket, monitor_height, monitor_width);
    
    add_monitor(monitor, canvas);


    char output_buffer[20];
    char n[10];
    sprintf(n,"%d", monitor->height);
    strcat(output_buffer,n);
    strcat(output_buffer,"x");
    sprintf(n,"%d",monitor->width);
    strcat(output_buffer,n);
    strcat(output_buffer,";");


    //sprintf(output_buffer, "%d", counter);
    int sev_val= send(client_socket, output_buffer, strlen(output_buffer), 0);
    printf("Respuesta enviada%d\n", sev_val);
    
    // pthread_mutex_lock(&conection_mutex);
    
    // pthread_mutex_unlock(&conection_mutex);

    //close(client_socket);
    saved_monitors++;
    pthread_exit(NULL);
    
    
}

void* handle_animation(void* arg){
    animation_thread_args* args =(animation_thread_args* )arg;
    move_object(args->objeto, args->canvas);

    //printf("animando \n");

    return NULL;
}

int main(int argc, char *argv[]) {
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

    Object** obj_list = NULL;
    int num_objects=0;
    load_config(config_file, canvas, &obj_list, &num_objects);

    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket falló");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind falló");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen falló");
        exit(EXIT_FAILURE);
    }

    
    printf("Servidor escuchando en el puerto %d...\n", PORT);

    int max_monitors = canvas->monitors_height*canvas->monitors_width;
    
    monitors_thread_args monitors_args[max_monitors];

    pthread_mutex_init(&conection_mutex, NULL);
    while (saved_monitors < max_monitors+1 && saved_monitors < canvas->amount_monitors) {
        int* new_socket = malloc(sizeof(int));
        *new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (*new_socket < 0) {
            perror("accept falló");
            free(new_socket);
            //continue;
        }
        monitors_args->canvas = canvas;
        monitors_args->socket = new_socket;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_monitors, monitors_args);
        pthread_detach(tid); // libera recursos automáticamente al terminar
        
    }
    
    pthread_mutex_destroy(&conection_mutex);
    close(server_fd);
    

    // for(int i =0; i<canvas->monitors_height;i++){
    //     for(int j=0; j<canvas->monitors_width;j++){
    //         if(canvas->monitors[i][j] != NULL){
    //             printf("%d, %d:", canvas->monitors[i][j]->id, canvas->monitors[i][j]->socket);
    //         }            
    //     }
    // }

    sleep(2);
    printf("\nhere\n");
    move_object(obj_list[0], canvas);
    return 0;
}
