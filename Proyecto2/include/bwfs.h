#ifndef BWFS_H
#define BWFS_H

#include <time.h>
#include <sys/types.h>
#include <stdint.h>

// --- Constantes del Sistema de Archivos ---

#define BWFS_MAGIC 0x42574653 // "BWFS" en ASCII
#define BLOCK_WIDTH 1000
#define BLOCK_HEIGHT 1000
#define BLOCK_SIZE (BLOCK_WIDTH * BLOCK_HEIGHT / 8) // Tamaño en bytes

#define MAX_FILENAME_LEN 255
#define MAX_PATH_LEN 4096

#define MAX_BLOCKS 256 // Máximo número de bloques de datos
#define MAX_INODES 256 // Máximo número de i-nodos
#define NUM_DIRECT_BLOCKS 12 // Número de bloques directos en un i-nodo

// --- Estructuras de Datos ---

// Superbloque: Metadatos globales del FS
typedef struct {
    uint32_t magic; // Número mágico para identificar el FS
    uint32_t total_blocks;
    uint32_t total_inodes;
    uint32_t inode_table_block; // Bloque donde empieza la tabla de i-nodos
    uint32_t block_bitmap_block; // Bloque para el bitmap de bloques
    uint32_t inode_bitmap_block; // Bloque para el bitmap de i-nodos
    // Podríamos añadir más campos si es necesario
} bwfs_superblock_t;

// I-nodo: Metadatos de un archivo o directorio
typedef struct {
    mode_t mode;       // Tipo de archivo (S_IFREG, S_IFDIR) y permisos
    uid_t uid;         // ID de usuario
    gid_t gid;         // ID de grupo
    off_t size;        // Tamaño en bytes
    time_t atime;      // Tiempo de último acceso
    time_t mtime;      // Tiempo de última modificación
    time_t ctime;      // Tiempo de último cambio de estado
    nlink_t nlink;     // Número de enlaces duros

    uint32_t direct_blocks[12]; // Punteros directos a bloques de datos
    // Para archivos más grandes, se necesitarían punteros indirectos
} bwfs_inode_t;

// Entrada de Directorio: Asocia un nombre con un i-nodo
typedef struct {
    char name[MAX_FILENAME_LEN + 1];
    uint32_t inode_num;
} bwfs_dir_entry_t;


#endif // BWFS_H
