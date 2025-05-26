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

int counter=0;
int saved_monitors=0;
typedef struct{
    int id;
    int socket;
    int height;
    int width;
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

pthread_mutex_t lock;

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

int load_config(const char* filename, Canvas* canvas, Object  ***arr, int *size){
    FILE *fp = fopen(filename, "r");
    if (!fp){
        perror("Error abriendo archivo");
        return -1;
    }

    //Define propiedades del canvas
    char line[30];
    fgets(line, 30, fp);
    printf("\n>%s",line);
    //ancho
    fgets(line, 10, fp);
    canvas->height  = atoi(line);
    printf("\n>%s",line);
    //alto
    fgets(line, 10, fp);
    canvas->width  = atoi(line);
    printf("\n>%s",line);

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
    printf("\n>%s",line);
    fgets(line, 14, fp);
    width_monitors = atoi(line);
    canvas->monitors_width = width_monitors;
    printf("\n>%s",line);

    for(int i=0;i<heigth_monitors;i++){
        for(int j=0;j<width_monitors;j++){
            Monitor *monitor = calloc(1, sizeof(Monitor));
            monitor->id = -1;
            canvas->monitors[heigth_monitors][width_monitors] = monitor;
        }
    }

    //Cantidad de objetos para la animacion
    int num_objects;
    fgets(line, 2, fp);
    fgets(line, 14, fp);
    fgets(line, 5, fp);
    
    num_objects = atoi(line);
    printf("\n>%s",line);
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


void* handle_client(void* arg) {
    counter++;
    int client_socket = *(int*)arg;
    free(arg);  // ya no necesitamos el puntero original

    char buffer[1024] = {0};
    char* hello = "Hello from server";

    read(client_socket, buffer, 1024);
    printf("Mensaje recibido: %s\n", buffer);
    send(client_socket, hello, strlen(hello), 0);
    printf("Respuesta enviada\n");
    printf("%d", client_socket);
    close(client_socket);
    saved_monitors++;
    pthread_exit(NULL);
    
    
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

    printf("\n>%d h:%d w:%d\n",-1, canvas->monitors_height,canvas->monitors_width);
    for(int i=0;i<num_objects;i++){
        
        printf("\n>%d x:%d y:%d\n",i, obj_list[i]->destined_x,obj_list[i]->destined_y);
    }
    
    printf("Servidor escuchando en el puerto %d...\n", PORT);

    int max_monitors= canvas->monitors_height*canvas->monitors_width;
    
    while (saved_monitors < max_monitors+1 && saved_monitors < canvas->amount_monitors) {
        printf("\n<<<a:%d b:%d\n", max_monitors, saved_monitors);
        int* new_socket = malloc(sizeof(int));
        *new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (*new_socket < 0) {
            perror("accept falló");
            free(new_socket);
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, new_socket);
        pthread_detach(tid); // libera recursos automáticamente al terminar
        
        printf("\na:%d b:%d>>>\n", max_monitors, saved_monitors);
    }
    
    printf("\n<<<a:%d b:%d c:%d>>>\n", max_monitors, saved_monitors, counter);
    
    close(server_fd);
    
    return 0;
}
