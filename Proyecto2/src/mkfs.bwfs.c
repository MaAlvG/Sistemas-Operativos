#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "bwfs.h"
#include "bwfs_image.h"

void print_usage(const char *prog_name) {
    fprintf(stderr, "Uso: %s <directorio>\n", prog_name);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *dir_path = argv[1];

    // Crear directorio si no existe
    struct stat st = {0};
    if (stat(dir_path, &st) == -1) {
        mkdir(dir_path, 0755);
    }

    // 1. Crear todos los bloques de imagen vacíos
    printf("Creando y formateando los bloques de imagen...\n");
    char filepath[MAX_PATH_LEN];
    for (uint32_t i = 0; i < MAX_BLOCKS; i++) {
        snprintf(filepath, sizeof(filepath), "%s/%u.png", dir_path, i);
        if (create_bw_image(filepath, BLOCK_WIDTH, BLOCK_HEIGHT) != 0) {
            fprintf(stderr, "Error: No se pudo crear el bloque de imagen %s\n", filepath);
            return 1;
        }
    }

    // 2. Preparar y escribir los metadatos usando un búfer de tamaño de bloque completo
    printf("Escribiendo metadatos...\n");
    char block_buffer[BLOCK_SIZE] = {0};

    // --- Escribir Superbloque (Bloque 0) ---
    bwfs_superblock_t *sb = (bwfs_superblock_t *)block_buffer;
    sb->magic = BWFS_MAGIC;
    sb->total_blocks = MAX_BLOCKS;
    sb->total_inodes = MAX_INODES;
    sb->inode_bitmap_block = 1;
    sb->block_bitmap_block = 2;
    sb->inode_table_block = 3;
    
    snprintf(filepath, sizeof(filepath), "%s/0.png", dir_path);
    if (write_data_to_image(filepath, (unsigned char *)block_buffer, BLOCK_SIZE) != 0) {
        fprintf(stderr, "Error: No se pudo escribir el superbloque.\n");
        return 1;
    }
    memset(block_buffer, 0, BLOCK_SIZE);

    // --- Escribir mapa de bits de i-nodos (Bloque 1) ---
    unsigned char *inode_bitmap = (unsigned char *)block_buffer;
    inode_bitmap[0] |= (1 << 0); // Marcar i-nodo 0 (raíz) como usado
    
    snprintf(filepath, sizeof(filepath), "%s/1.png", dir_path);
    if (write_data_to_image(filepath, (unsigned char *)block_buffer, BLOCK_SIZE) != 0) {
        fprintf(stderr, "Error: No se pudo escribir el mapa de bits de i-nodos.\n");
        return 1;
    }
    memset(block_buffer, 0, BLOCK_SIZE);

    // --- Escribir mapa de bits de bloques (Bloque 2) ---
    unsigned char *block_bitmap = (unsigned char *)block_buffer;
    // 0: Super, 1: Inode bitmap, 2: Block bitmap, 3: Inode table, 4: Root data
    block_bitmap[0] = 0b00011111; // Marcar los primeros 5 bloques como usados
    
    snprintf(filepath, sizeof(filepath), "%s/2.png", dir_path);
    if (write_data_to_image(filepath, (unsigned char *)block_buffer, BLOCK_SIZE) != 0) {
        fprintf(stderr, "Error: No se pudo escribir el mapa de bits de bloques.\n");
        return 1;
    }
    memset(block_buffer, 0, BLOCK_SIZE);

    // --- Escribir tabla de i-nodos (Bloque 3) ---
    bwfs_inode_t *inode_table = (bwfs_inode_t *)block_buffer;
    inode_table[0].mode = S_IFDIR | 0755;
    inode_table[0].uid = getuid();
    inode_table[0].gid = getgid();
    inode_table[0].nlink = 2;
    inode_table[0].size = sizeof(bwfs_dir_entry_t) * 2;
    inode_table[0].atime = inode_table[0].mtime = inode_table[0].ctime = time(NULL);
    inode_table[0].direct_blocks[0] = 4; // Apunta al bloque de datos del dir raíz
    
    snprintf(filepath, sizeof(filepath), "%s/3.png", dir_path);
    if (write_data_to_image(filepath, (unsigned char *)block_buffer, BLOCK_SIZE) != 0) {
        fprintf(stderr, "Error: No se pudo escribir la tabla de i-nodos.\n");
        return 1;
    }
    memset(block_buffer, 0, BLOCK_SIZE);

    // --- Escribir bloque de datos del directorio raíz (Bloque 4) ---
    bwfs_dir_entry_t *entries = (bwfs_dir_entry_t *)block_buffer;
    strcpy(entries[0].name, ".");
    entries[0].inode_num = 0;
    strcpy(entries[1].name, "..");
    entries[1].inode_num = 0; // En la raíz, '..' apunta a sí mismo
    
    snprintf(filepath, sizeof(filepath), "%s/4.png", dir_path);
    if (write_data_to_image(filepath, (unsigned char *)block_buffer, BLOCK_SIZE) != 0) {
        fprintf(stderr, "Error: No se pudo escribir el bloque de datos del dir raíz.\n");
        return 1;
    }

    printf("¡Sistema de archivos BWFS creado exitosamente en '%s'!\n", dir_path);
    return 0;
}
