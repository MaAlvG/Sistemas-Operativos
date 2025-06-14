#ifndef BWFS_H
#define BWFS_H

#include <stdint.h>
#include <time.h>
#include <sys/types.h>

// Constantes del sistema de archivos
#define BWFS_BLOCK_SIZE 1000000     // 1000x1000 pixels = 1M bits = 125KB
#define BWFS_MAX_FILENAME 255       // Longitud máxima del nombre de archivo
#define BWFS_MAX_FILES 1000         // Máximo número de archivos/inodos
#define BWFS_SIGNATURE "BWFS"       // Firma del sistema de archivos
#define BWFS_VERSION 1              // Versión actual
#define BWFS_MAX_BLOCKS_PER_FILE 16 // Máximo bloques directos por archivo

// Tipos de inodos
#define BWFS_INODE_FILE 0
#define BWFS_INODE_DIR 1

// Estructura del superbloque
typedef struct {
    char signature[4];          // Firma "BWFS"
    uint32_t version;           // Versión del formato
    uint32_t block_size;        // Tamaño del bloque en bits
    uint32_t total_blocks;      // Total de bloques en el FS
    uint32_t free_blocks;       // Bloques libres
    uint32_t inode_count;       // Número de inodos usados
    uint32_t root_inode;        // Número del inodo raíz
    time_t created;             // Fecha de creación
    time_t modified;            // Última modificación
    uint32_t flags;             // Flags del sistema de archivos
    uint8_t reserved[476];      // Espacio reservado para futuras extensiones
} __attribute__((packed)) bwfs_superblock_t;

// Estructura del inodo
typedef struct {
    uint32_t inode_num;         // Número del inodo
    uint32_t type;              // Tipo (archivo=0, directorio=1)
    uint32_t size;              // Tamaño en bytes
    uint32_t blocks[BWFS_MAX_BLOCKS_PER_FILE]; // Bloques directos
    uint32_t indirect_block;    // Bloque indirecto (para archivos grandes)
    time_t atime;               // Último acceso
    time_t mtime;               // Última modificación
    time_t ctime;               // Cambio de metadatos
    uint32_t links;             // Número de enlaces
    uint32_t uid;               // ID del propietario
    uint32_t gid;               // ID del grupo
    uint32_t mode;              // Permisos y tipo
    uint32_t flags;             // Flags del inodo
    uint8_t reserved[32];       // Espacio reservado
} __attribute__((packed)) bwfs_inode_t;

// Entrada de directorio
typedef struct {
    uint32_t inode;             // Número del inodo
    char name[BWFS_MAX_FILENAME]; // Nombre del archivo
    uint8_t type;               // Tipo de entrada
    uint8_t reserved[4];        // Alineación
} __attribute__((packed)) bwfs_dirent_t;

// Estructura para manejo de bloques libres
typedef struct {
    uint32_t next_free;         // Siguiente bloque libre
    uint8_t reserved[124];      // Resto del bloque
} __attribute__((packed)) bwfs_free_block_t;

// Estructura para bloques indirectos
typedef struct {
    uint32_t blocks[31250];     // Referencias a bloques de datos (125KB / 4 bytes)
} __attribute__((packed)) bwfs_indirect_block_t;

// Metadatos del sistema de archivos
typedef struct {
    bwfs_superblock_t *superblock;
    bwfs_inode_t *inode_table;
    uint32_t *block_bitmap;
    char *base_path;
    int mounted;
} bwfs_context_t;

// Funciones principales del sistema de archivos
int bwfs_format(const char *path, uint32_t total_blocks);
int bwfs_mount(const char *fs_path, const char *mount_point);
int bwfs_unmount(const char *mount_point);
int bwfs_check(const char *path);

// Funciones de manejo de bloques
int bwfs_write_block(uint32_t block_num, const uint8_t *data, size_t size);
int bwfs_read_block(uint32_t block_num, uint8_t *data, size_t size);
uint32_t bwfs_allocate_block(void);
void bwfs_free_block(uint32_t block_num);
char* bwfs_get_block_filename(uint32_t block_num);

// Funciones de manejo de inodos
uint32_t bwfs_allocate_inode(void);
void bwfs_free_inode(uint32_t inode_num);
bwfs_inode_t* bwfs_get_inode(uint32_t inode_num);

// Funciones de manejo de imágenes PNG
int bwfs_write_png_block(const char *filename, const uint8_t *data, int width, int height);
int bwfs_read_png_block(const char *filename, uint8_t *data, int *width, int *height);

// Funciones de utilidad
int bwfs_path_to_inode(const char *path, uint32_t *inode_num);
int bwfs_add_dir_entry(uint32_t dir_inode, const char *name, uint32_t file_inode, uint8_t type);
int bwfs_remove_dir_entry(uint32_t dir_inode, const char *name);
int bwfs_list_dir(uint32_t dir_inode, bwfs_dirent_t *entries, int max_entries);

// Funciones de fragmentación
int bwfs_defragment(const char *path);
float bwfs_get_fragmentation_ratio(const char *path);
int bwfs_compact_blocks(const char *path);

// Funciones de validación
int bwfs_validate_superblock(const bwfs_superblock_t *sb);
int bwfs_validate_inode(const bwfs_inode_t *inode);
int bwfs_validate_block_num(uint32_t block_num);

// Macros útiles
#define BWFS_BLOCK_TO_BYTES(blocks) ((blocks) * (BWFS_BLOCK_SIZE / 8))
#define BWFS_BYTES_TO_BLOCKS(bytes) (((bytes) + (BWFS_BLOCK_SIZE / 8) - 1) / (BWFS_BLOCK_SIZE / 8))
#define BWFS_IS_VALID_INODE(inum) ((inum) > 0 && (inum) < BWFS_MAX_FILES)
#define BWFS_IS_FILE(inode) ((inode)->type == BWFS_INODE_FILE)
#define BWFS_IS_DIR(inode) ((inode)->type == BWFS_INODE_DIR)

// Estados de error
#define BWFS_ERROR_OK           0
#define BWFS_ERROR_INVALID     -1
#define BWFS_ERROR_NOT_FOUND   -2
#define BWFS_ERROR_NO_SPACE    -3
#define BWFS_ERROR_IO          -4
#define BWFS_ERROR_CORRUPT     -5
#define BWFS_ERROR_EXISTS      -6
#define BWFS_ERROR_NOT_EMPTY   -7

#endif // BWFS_H
