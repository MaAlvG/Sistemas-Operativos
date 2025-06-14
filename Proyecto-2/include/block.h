#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>
#include <stddef.h>
#include <time.h>  // Para time_t

// Tamaño de bloque en bytes
#define BLOCK_SIZE 4096

// Número máximo de bloques en el sistema de archivos
#define MAX_BLOCKS 1024

// Estructura de contexto de bloques
typedef struct {
    char *base_path;           // Ruta base para almacenamiento de bloques
    uint8_t *block_bitmap;     // Mapa de bits de bloques libres/ocupados
    size_t total_blocks;       // Número total de bloques
    size_t free_blocks;        // Número de bloques libres
    // Superbloque integrado en el contexto
    struct {
        char signature[8];      // Firma del sistema de archivos
        uint32_t version;         // Versión del sistema de archivos
        uint32_t block_size;      // Tamaño de cada bloque
        uint32_t total_blocks;    // Total de bloques en el sistema
        uint32_t free_blocks;     // Contador de bloques libres
        uint32_t inode_count;     // Número de inodos
        uint32_t root_inode;      // Número de inodo raíz
        time_t created;           // Marca de tiempo de creación
        time_t modified;          // Última modificación
    } superblock;
} block_ctx_t;

// Contexto global
extern block_ctx_t block_ctx;

// Declaraciones de funciones
int block_init(const char *base_path, size_t total_blocks);
void block_cleanup(void);
uint32_t block_allocate(void);
void block_free(uint32_t block_num);
int block_is_free(uint32_t block_num);
int block_read(uint32_t block_num, void *buffer);
int block_write(uint32_t block_num, const void *data);

#endif // BLOCK_H
