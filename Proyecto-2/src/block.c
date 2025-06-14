#include "../include/bwfs.h"
#define _POSIX_C_SOURCE 200809L  // Para strdup
#include "../include/block.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

// Contexto global de bloques
block_ctx_t block_ctx = {0};

// Contexto global de BWFS
bwfs_context_t ctx = {0};

// Duplicación segura de cadenas
static char* safe_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *new = malloc(len);
    if (new) memcpy(new, s, len);
    return new;
}

// Inicializar la gestión de bloques
int block_init(const char *base_path, size_t total_blocks) {
    printf("Inicializando gestor de bloques con ruta: %s\n", base_path);
    
    // Validar parámetros
    if (!base_path || total_blocks == 0 || total_blocks > MAX_BLOCKS) {
        fprintf(stderr, "Error: Parámetros inválidos (ruta: %p, bloques totales: %zu)\n", 
                (void*)base_path, total_blocks);
        return -1;
    }
    
    // Inicializar contexto
    memset(&block_ctx, 0, sizeof(block_ctx));
    
    // Configurar superbloque
    memcpy(block_ctx.superblock.signature, "BWFSv1\0\0\0", 8);
    block_ctx.superblock.version = 1;
    block_ctx.superblock.block_size = BLOCK_SIZE;
    block_ctx.superblock.total_blocks = total_blocks;
    block_ctx.superblock.free_blocks = total_blocks - 2; // Reservar primeros dos bloques
    block_ctx.superblock.inode_count = 1;  // Inodo raíz
    block_ctx.superblock.root_inode = 1;
    time(&block_ctx.superblock.created);
    time(&block_ctx.superblock.modified);
    
    // Configurar seguimiento de bloques
    block_ctx.total_blocks = total_blocks;
    block_ctx.free_blocks = total_blocks - 2;  // Reservar primeros dos bloques
    
    // Duplicar ruta base
    block_ctx.base_path = safe_strdup(base_path);
    if (!block_ctx.base_path) {
        fprintf(stderr, "Error: No se pudo duplicar la ruta base\n");
        return -1;
    }
    
    // Inicializar mapa de bits de bloques
    size_t bitmap_size = (total_blocks + 7) / 8;
    block_ctx.block_bitmap = calloc(1, bitmap_size);
    if (!block_ctx.block_bitmap) {
        fprintf(stderr, "Error: No se pudo asignar el mapa de bits de bloques\n");
        free(block_ctx.base_path);
        block_ctx.base_path = NULL;
        return -1;
    }
    
    // Marcar bloques del superbloque y del inodo raíz como ocupados
    block_ctx.block_bitmap[0] = 0x03;  // Primeros dos bloques (0 y 1) están ocupados
    
    printf("Gestor de bloques inicializado exitosamente con %zu bloques\n", total_blocks);
    return 0;
}

// Liberar recursos del gestor de bloques
void block_cleanup() {
    free(block_ctx.block_bitmap);
    free(block_ctx.base_path);
    memset(&block_ctx, 0, sizeof(block_ctx));
}

// Obtener nombre de archivo para un bloque
static char* block_get_filename(uint32_t block_num) {
    static char filename[1024];
    snprintf(filename, sizeof(filename), "%s/block_%06u.dat", block_ctx.base_path, block_num);
    return filename;
}

// Verificar si un bloque está libre
int block_is_free(uint32_t block_num) {
    if (block_num >= block_ctx.total_blocks) {
        fprintf(stderr, "Error: Número de bloque %u fuera de rango (máx %zu)\n", 
                block_num, block_ctx.total_blocks);
        return 0;
    }
    return !(block_ctx.block_bitmap[block_num / 8] & (1 << (block_num % 8)));
}

// Marcar un bloque como ocupado
static void block_mark_used(uint32_t block_num) {
    if (block_num < block_ctx.total_blocks) {
        block_ctx.block_bitmap[block_num / 8] |= (1 << (block_num % 8));
        if (block_ctx.free_blocks > 0) {
            block_ctx.free_blocks--;
        }
    }
}

// Marcar un bloque como libre
static void block_mark_free(uint32_t block_num) {
    if (block_num < block_ctx.total_blocks) {
        block_ctx.block_bitmap[block_num / 8] &= ~(1 << (block_num % 8));
        block_ctx.free_blocks++;
    }
}

// Asignar un nuevo bloque
uint32_t block_allocate() {
    if (!block_ctx.block_bitmap) {
        fprintf(stderr, "Error: Gestor de bloques no inicializado\n");
        return 0;
    }
    
    if (block_ctx.free_blocks == 0) {
        fprintf(stderr, "Error: No hay bloques libres disponibles\n");
        return 0;
    }
    
    printf("Asignando nuevo bloque (libres: %zu/%zu)\n", 
           block_ctx.free_blocks, block_ctx.total_blocks);
    
    // Asignación simple de primer ajuste
    for (uint32_t i = 2; i < block_ctx.total_blocks; i++) {  // Saltar primeros dos bloques
        if (block_is_free(i)) {
            printf("Bloque libre encontrado: %u\n", i);
            block_mark_used(i);
            return i;
        }
    }
    
    fprintf(stderr, "Error: No se encontraron bloques libres (estado inconsistente)\n");
    return 0; // No debería llegar aquí si free_blocks es preciso
}

// Liberar un bloque
void block_free(uint32_t block_num) {
    block_mark_free(block_num);
}

// Leer un bloque del disco
int block_read(uint32_t block_num, void *buffer) {
    if (block_num >= block_ctx.total_blocks) {
        fprintf(stderr, "Error: Número de bloque %u fuera de rango (máx %zu)\n", 
                block_num, block_ctx.total_blocks - 1);
        return -1;
    }
    
    if (!buffer) {
        fprintf(stderr, "Error: Puntero de buffer nulo\n");
        return -1;
    }
    
    char *filename = block_get_filename(block_num);
    if (!filename) {
        fprintf(stderr, "Error: No se pudo obtener el nombre de archivo para el bloque %u\n", block_num);
        return -1;
    }
    
    printf("Leyendo del bloque %u en %s\n", block_num, filename);
    
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error al abrir el archivo de bloque");
        return -1;
    }
    
    ssize_t read_bytes = read(fd, buffer, BLOCK_SIZE);
    close(fd);
    
    if (read_bytes != BLOCK_SIZE) {
        fprintf(stderr, "Error al leer el bloque completo (leídos %zd bytes)\n", read_bytes);
        return -1;
    }
    
    printf("Bloque %u leído exitosamente\n", block_num);
    return 0;
}

// Escribir un bloque en el disco
int block_write(uint32_t block_num, const void *data) {
    if (block_num >= block_ctx.total_blocks) {
        fprintf(stderr, "Error: Número de bloque %u fuera de rango (máx %zu)\n", 
                block_num, block_ctx.total_blocks - 1);
        return -1;
    }
    
    if (!data) {
        fprintf(stderr, "Error: Null data pointer\n");
        return -1;
    }
    
    char *filename = block_get_filename(block_num);
    if (!filename) {
        fprintf(stderr, "Error: Failed to get filename for block %u\n", block_num);
        return -1;
    }
    
    printf("Writing to block %u at %s\n", block_num, filename);
    
    // Ensure directory exists
    if (mkdir(block_ctx.base_path, 0755) < 0 && errno != EEXIST) {
        perror("Failed to create directory");
        return -1;
    }
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to open block file");
        return -1;
    }
    
    ssize_t written = write(fd, data, BLOCK_SIZE);
    close(fd);
    
    if (written != BLOCK_SIZE) {
        fprintf(stderr, "Failed to write full block (wrote %zd bytes)\n", written);
        return -1;
    }
    
    printf("Successfully wrote block %u\n", block_num);
    return 0;
}
