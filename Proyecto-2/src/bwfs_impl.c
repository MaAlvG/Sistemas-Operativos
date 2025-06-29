#define _POSIX_C_SOURCE 200809L  // Para struct timespec
#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <sys/stat.h>   // Para S_IFDIR, S_IFREG, etc.
#include <sys/types.h>  // Para mode_t
#include <time.h>       // Para time()
#include <unistd.h>
#include <stdlib.h>  // Para realpath, free
#include <limits.h>  // Para PATH_MAX
#include "../include/bwfs.h"

// Contexto global
static struct {
    const char *root_dir;
    bwfs_superblock_t superblock;
} bwfs_data;

// Obtener atributos de un archivo o directorio
static int bwfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;  // No usado por ahora
    int res = 0;
    
    // Inicializar la estructura stat con ceros
    memset(stbuf, 0, sizeof(struct stat));
    
    // Establecer el UID y GID del usuario actual
    stbuf->st_uid = getuid();
    stbuf->st_gid = getgid();
    
    // Establecer la hora de último acceso, modificación y cambio
    time_t now = time(NULL);
    stbuf->st_atime = now;  // Tiempo de último acceso
    stbuf->st_mtime = now;  // Tiempo de última modificación
    stbuf->st_ctime = now;  // Tiempo de último cambio de estado
    
    if (strcmp(path, "/") == 0) {
        // Es el directorio raíz
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;  // Por el . y ..
        stbuf->st_size = 4096; // Tamaño típico de un directorio
    } else if (strcmp(path, "/hola.txt") == 0) {
        // Es el archivo hola.txt
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen("Hola, BWFS!\n");
    } else {
        // Archivo no encontrado
        res = -ENOENT;
    }
    
    return res;
}

// Leer directorio
static int bwfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    // Aquí se pueden añadir más entradas de directorio

    return 0;
}

// Crear un nuevo archivo
static int bwfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    (void) fi;  // No usado por ahora
    (void) mode; // No usado por ahora
    
    printf("Creando archivo: %s\n", path);
    
    // En una implementación real, aquí crearíamos un nuevo inodo y lo añadiríamos
    // al directorio correspondiente
    
    // Retornamos 0 para indicar éxito
    return 0;
}

// Escribir datos en un archivo
static int bwfs_write(const char *path, const char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi) {
    (void) fi; // No usado por ahora
    
    printf("Escribiendo %zu bytes en %s en la posición %ld\n", size, path, (long)offset);
    printf("Contenido: %.*s\n", (int)size, buf);
    
    // En una implementación real, aquí escribiríamos los datos en el archivo
    // correspondiente al path
    
    // Retornamos el número de bytes escritos
    return size;
}

// Abrir un archivo
static int bwfs_open(const char *path, struct fuse_file_info *fi) {
    printf("Abriendo archivo: %s\n", path);
    
    // En una implementación real, aquí verificaríamos los permisos y 
    // configuraríamos el archivo si es necesario
    
    // Por ahora, simplemente retornamos éxito
    return 0;
}

// Leer datos de un archivo
static int bwfs_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
    (void) fi; // No usado por ahora
    
    printf("Leyendo %zu bytes de %s desde la posición %ld\n", size, path, (long)offset);
    
    // Verificar qué archivo se está leyendo
    if (strcmp(path, "/hola.txt") == 0) {
        const char *contenido = "Hola, BWFS!\n";
        size_t len = strlen(contenido);
        
        // Asegurarse de no leer más allá del final del archivo
        if ((size_t)offset >= len) {
            return 0; // Fin de archivo
        }
        
        // Ajustar el tamaño de lectura si es necesario
        if ((size_t)offset + size > len) {
            size = len - (size_t)offset;
        }
        
        // Copiar los datos al buffer
        memcpy(buf, contenido + offset, size);
        return size;
    } else if (strcmp(path, "/") == 0) {
        // Es un directorio, no se puede leer
        return -EISDIR;
    } else {
        // Archivo no encontrado
        return -ENOENT;
    }
}

// Operaciones de FUSE
static struct fuse_operations bwfs_oper = {
    .getattr    = bwfs_getattr,
    .readdir    = bwfs_readdir,
    .create     = bwfs_create,
    .write      = bwfs_write,
    .read       = bwfs_read,
    .open       = bwfs_open,
    // Otras operaciones se pueden añadir aquí
};

int main(int argc, char *argv[]) {
    // Verificar argumentos
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <directorio> <punto_montaje> [opciones]\n", argv[0]);
        fprintf(stderr, "Opciones comunes:\n");
        fprintf(stderr, "   -d   Habilita mensajes de depuración\n");
        fprintf(stderr, "   -f   Ejecuta en primer plano\n");
        return 1;
    }

    // Inicializar la estructura de datos
    bwfs_data.root_dir = strdup(argv[1]);
    if (!bwfs_data.root_dir) {
        perror("Error al asignar memoria para el directorio raíz");
        return 1;
    }

    // Crear una copia de los argumentos para FUSE
    // El primer argumento debe ser el nombre del programa
    // El resto de argumentos se pasan directamente a FUSE
    char **fuse_argv = (char **)malloc((argc + 1) * sizeof(char *));
    if (!fuse_argv) {
        perror("Error al asignar memoria para los argumentos");
        free((void*)bwfs_data.root_dir);
        return 1;
    }

    // El primer argumento es el nombre del programa
    fuse_argv[0] = argv[0];
    
    // El resto de argumentos se copian tal cual
    for (int i = 1; i < argc; i++) {
        fuse_argv[i] = argv[i];
    }
    fuse_argv[argc] = NULL;

    // Mostrar información de depuración
    printf("Montando BWFS:\n");
    printf("  Directorio raíz: %s\n", bwfs_data.root_dir);
    printf("  Punto de montaje: %s\n", argv[2]);

    // Iniciar FUSE
    // Usamos argc-1 y fuse_argv+1 para omitir el nombre del programa
    int res = fuse_main(argc-1, fuse_argv+1, &bwfs_oper, NULL);

    // Liberar memoria
    free((void*)bwfs_data.root_dir);
    free(fuse_argv);

    return res;
}
