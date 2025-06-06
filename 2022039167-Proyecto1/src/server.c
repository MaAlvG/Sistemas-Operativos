#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>

#define MAX_CANVAS_SIZE 1001
#define PORT 8080
#define MAX_OBJECT_SIZE 30
#define MAX_MONITORS 6
#define MAX_MONITOR_HEIGHT 31
#define MAX_MONITOR_WIDTH 128

int counter=0;

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
    char canvas_drawing[MAX_CANVAS_SIZE][MAX_CANVAS_SIZE];
    Monitor *monitors[MAX_MONITORS][MAX_MONITORS];
    int amount_monitors;
} Canvas;

typedef struct{
    char* id;
    int x, y;                                       // Posición actual
    int width, height;                              // Tamaño
    char drawing[MAX_OBJECT_SIZE][MAX_OBJECT_SIZE];// Dibujo (matriz de ASCII)
    int rotations;                                      // Rotación:
    // char *schedulerType;
    double start_time, end_time; // Tiempo de aparición y desaparición
    int destined_x, destined_y; // Movimiento por frame
    int active; 
    int movements ;             // Si está activo o esperando
} Object;

typedef struct {
    Object** obj_list;
    Canvas* canvas;
    int num_objects;
} checker_thread_args;

typedef struct{
    Object* objeto;
    Canvas* canvas;
} animation_thread_args;

pthread_mutex_t canvas_send_mutex = PTHREAD_MUTEX_INITIALIZER;
struct timespec ts;

double program_starting_time;

// Función para obtener el tiempo actual en segundos
double get_current_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}


/*identificador, x de inicio, y de inicio, x final, y final, tiempo de inicio, tiempo de final,*/
Object *init_object(char* id, int x, int y, int dx, int dy, double sT, double eT, int rot){
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
    pthread_mutex_init(&monitor->monitor_lock, NULL);

    return monitor;
}

int load_config(const char* filename, Canvas* canvas, Object  ***arr, int *size){

    char path[256];
    snprintf(path, sizeof(path), "config/%s", filename);
    FILE *fp = fopen(path, "r");

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
            canvas->canvas_drawing[i][j]= ' ';

        }
    }
    
    
    //Disposicion de los monitores
    int amount_monitors,heigth_monitors, width_monitors;
    fgets(line, 2, fp);
    fgets(line, 22, fp);
    fgets(line, 22, fp);
    fgets(line, 14, fp);
    amount_monitors = atoi(line); // cantidad de monitores en total
    canvas->amount_monitors = amount_monitors;

    fgets(line, 14, fp);
    heigth_monitors = atoi(line);//cantidad de monitores en vertical posibles
    canvas->monitors_height = heigth_monitors;//cantidad de monitores en vertical posibles
    
    fgets(line, 14, fp);
    width_monitors = atoi(line);//cantidad de monitores en horizontal posibles
    canvas->monitors_width = width_monitors;//cantidad de monitores en horizontal posibles

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
        double start_time;
        double end_time;
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
        
        start_time = atof(line);
        start_time = start_time+ program_starting_time;
        fgets(line, 2, fp);
        fgets(line, 14, fp);
        fgets(line, 5, fp);
        
        end_time = atof(line);
        end_time = end_time + program_starting_time;

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
    
    //printf("\n{%d %d}\n",y_monitors, x_monitors);
    for(int i =0; i<y_monitors;i++){
        for(int j=0; j<x_monitors;j++){
            if(canvas->monitors[i][j]== NULL){
                canvas->monitors[i][j]= monitor;
                //printf("\n[%d %d]\n",i, j);
                i= y_monitors+1;
                j= x_monitors+1;
                
            }
        }   
    }
}

void update_locks(int step_x,int step_y,Object *obj, Canvas* canvas){
    int start_x = obj->x;
    int start_y = obj->y;
    if(!obj->active) return;
    
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

void draw_object(Object *obj,Canvas *canvas){
    int start_x = obj->x;
    int start_y = obj->y;

    for (int i = 0; i < obj->height; i++){
        for (int j = 0; j < obj->width; j++){
            canvas->canvas_drawing[start_y + i][start_x + j]=obj->drawing[i][j];
            
        }
    }
}

void erase_object(Object *obj,Canvas *canvas){
    int start_x = obj->x;
    int start_y = obj->y;

    for (int i = 0; i < obj->height; i++){
        for (int j = 0; j < obj->width; j++){
            
            canvas->canvas_drawing[start_y + i][start_x + j]=' ';            
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

// Función para envío confiable con reintentos
int reliable_send(int socket, const char* data, size_t len) {
    size_t total_sent = 0;
    int attempts = 0;
    const int max_attempts = 5;
    
    while(total_sent < len && attempts < max_attempts) {
        int sent = send(socket, data + total_sent, len - total_sent, 0);
        
        if(sent < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                // Buffer lleno, esperar un poco
                usleep(1000); // 1ms
                attempts++;
                continue;
            } else {
                // Error real de conexión
                printf("NANI?");
                return -1;
            }
        } else if(sent == 0) {
            printf("EH?");
            // Conexión cerrada
            return -1;
        }
        
        total_sent += sent;
        attempts = 0; // Reiniciar los intentos
    }
    
    return (total_sent == len) ? 0 : -1;
}

// Envío del canvas por lotes 
void send_print(Canvas *canvas) {
    
    // Crear buffer por monitor para envío en lotes
    char monitor_buffers[MAX_MONITORS][MAX_MONITORS][4096];
    int buffer_lengths[MAX_MONITORS][MAX_MONITORS];
    
    // Inicializar buffers
    for(int mi = 0; mi < canvas->monitors_height; mi++) {
        for(int mj = 0; mj < canvas->monitors_width; mj++) {
            monitor_buffers[mi][mj][0] = '\0';
            buffer_lengths[mi][mj] = 0;
        }
    }
    
    // Construir lotes de comandos por monitor
    for(int i = 0; i < canvas->height; i++) {
        for(int j = 0; j < canvas->width; j++) {
            int monitor_row = i / MAX_MONITOR_HEIGHT;
            int monitor_col = j / MAX_MONITOR_WIDTH;
            
            if(monitor_row >= canvas->monitors_height || 
               monitor_col >= canvas->monitors_width ||
               canvas->monitors[monitor_row][monitor_col] == NULL) {
                continue;
            }
            
            int local_row = (i % MAX_MONITOR_HEIGHT) + 1;
            int local_col = (j % MAX_MONITOR_WIDTH) + 1;
            
            char command[32];
            int cmd_len = sprintf(command, "PRINT:%d,%d,%c;", 
                                local_row, local_col, canvas->canvas_drawing[i][j]);
            
            // Verificar espacio en buffer
            if(buffer_lengths[monitor_row][monitor_col] + cmd_len < 4000) {
                strcat(monitor_buffers[monitor_row][monitor_col], command);
                buffer_lengths[monitor_row][monitor_col] += cmd_len;
            } else {
                // Buffer lleno, enviar y resetear
                Monitor *monitor = canvas->monitors[monitor_row][monitor_col];
                if(reliable_send(monitor->socket, 
                                monitor_buffers[monitor_row][monitor_col], 
                                buffer_lengths[monitor_row][monitor_col]) < 0) {
                    printf("Error enviando lote a monitor %d\n", monitor->id);
                }
                
                // Resetear buffer y agregar comando actual
                strcpy(monitor_buffers[monitor_row][monitor_col], command);
                buffer_lengths[monitor_row][monitor_col] = cmd_len;
            }
        }
    }
    
    // Enviar buffers restantes
    for(int mi = 0; mi < canvas->monitors_height; mi++) {
        for(int mj = 0; mj < canvas->monitors_width; mj++) {
            if(canvas->monitors[mi][mj] != NULL && buffer_lengths[mi][mj] > 0) {
                Monitor *monitor = canvas->monitors[mi][mj];
                if(reliable_send(monitor->socket, 
                                monitor_buffers[mi][mj], 
                                buffer_lengths[mi][mj]) < 0) {
                    printf("Error enviando lote final a monitor %d\n", monitor->socket);
                }
            }
        }
    }
    
}

void explode(Object *obj){
    int half = obj->height/2;

    int identation= (obj->width-4)/2;

    for (int i = 0; i < obj->height; i++){
        for (int j = 0; j < obj->width; j++){
            
            obj->drawing[i][j]=' ';            
        }
    }

    obj->drawing[half][identation]= 'B';
    obj->drawing[half][identation+1]= 'O';
    obj->drawing[half][identation+2]= 'O';
    obj->drawing[half][identation+3]= 'M';
}

void check_explode(Object *obj, Canvas *canvas) {
    double current_time = get_current_time();
    
    if (current_time > obj->end_time && obj->active) {
        printf("BOOM! Objeto %s explotó en posición (%d, %d)\n", obj->id, obj->x, obj->y);
        
        // Crear explosión visual
        explode(obj);

        // Marcar como inactivo y liberar mutex
        release_locks(obj, canvas);        
        obj->active = 0;
    }
}

void move_object(Object* obj, Canvas* canvas) {
    int move_flag = 1;
    
    double current_time = get_current_time();
    double start_time = obj->start_time;
    while(start_time >= current_time){
        usleep(50000);
        current_time = get_current_time();
    }

    while(obj->active) {
        int step_x = 0, step_y = 0;

        // Calcular paso
        if(obj->destined_x > obj->x) step_x = 1;
        else if(obj->destined_x < obj->x) step_x = -1;
        
        if(obj->destined_y > obj->y) step_y = 1;
        else if(obj->destined_y < obj->y) step_y = -1;

        // Modificar canvas con protección
        pthread_mutex_trylock(&canvas_send_mutex);
        
        erase_object(obj, canvas);
        
        if(step_x != 0) obj->x += step_x;
        if(step_y != 0) obj->y += step_y;
        
        if(obj->rotations > 0) rotate(obj, obj->rotations);
        
        draw_object(obj, canvas);
        
        pthread_mutex_unlock(&canvas_send_mutex);
        update_locks(step_x, step_y, obj, canvas);
        // Enviar frame completo en lotes
        send_print(canvas);
        
        obj->active = (obj->destined_x != obj->x) || (obj->destined_y != obj->y);
        

        usleep(70000);
    }
    release_locks(obj, canvas);
    erase_object(obj, canvas);
    
}

int connect_monitors(int *socket, Canvas *canvas){

    int client_socket = *socket;
    char input_buffer[1024] = {0};


    int read_input = recv(client_socket, input_buffer, 1023,0);
    int monitor_height=31;
    int monitor_width=128;
    
    if(read_input==-1){
        printf("conexion invalida");
        return -1;
    }

    Monitor *monitor = new_monitor(counter, client_socket, monitor_height, monitor_width);
    
    add_monitor(monitor, canvas);


    char *output_buffer= "Conexion establecida";

    //sprintf(output_buffer, "%d", counter);
    int sev_val= send(client_socket, output_buffer, strlen(output_buffer), 0);
    printf("Respuesta enviada %s\n", output_buffer);
    
    //close(client_socket);
   return 0;
}

void* handle_animation(void* arg){
    animation_thread_args* args =(animation_thread_args* )arg;
    move_object(args->objeto, args->canvas);

    return NULL;
}

void* handle_checker(void* arg){
    int flag = 1;
    checker_thread_args* args =(checker_thread_args* )arg;
    Object ** obj_list = args->obj_list;
    Canvas* canvas = args->canvas;
    int num_objects = args->num_objects;

    while(flag){
        flag =0;
        for(int i =0; i < num_objects;i++){
            check_explode(obj_list[i], canvas);
            flag = flag || obj_list[i]->active;
        }

    }
    sleep(1);         
    
    return NULL;
}

void end_animation(Canvas * canvas){
    for(int i =0; i<canvas->monitors_height;i++){
        for(int j=0; j<canvas->monitors_width;j++){
            if(canvas->monitors[i][j] != NULL){
                send(canvas->monitors[i][j]->socket, "END;", 6,0);
            }            
        }
    }
    
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

    program_starting_time = get_current_time();
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

    while (counter < canvas->amount_monitors) {
        int* new_socket = malloc(sizeof(int));
        *new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (*new_socket < 0) {
            perror("accept falló");
            free(new_socket);
            continue;
        }

        int result = connect_monitors(new_socket, canvas);
        if(result == 0){
            counter++;
        }

    }

    pthread_t threads[num_objects];
    animation_thread_args args[num_objects];
    sleep(1);
    
    pthread_t checker;
    checker_thread_args arguments;

    arguments.obj_list = obj_list;
    arguments.canvas = canvas;
    arguments.num_objects = num_objects;

    pthread_create(&checker,NULL,handle_checker, &arguments);

    for(int i=0;i< num_objects;i++){

        args[i].canvas = canvas;
        args[i].objeto = obj_list[i];
        pthread_create(&threads[i],NULL,handle_animation, &args[i]);
    }
    

    for (int i = 0; i < num_objects; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_join(checker, NULL);

    printf("\n animacion terminada\n");
    sleep(1);
    end_animation(canvas);
    close(server_fd);
    return 0;
}

