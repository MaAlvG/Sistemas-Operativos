#define FUSE_USE_VERSION 31
#define MKFS_BWFS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>  // Para memset
#include <errno.h>
#include "../include/bwfs.h"

// Función para crear el directorio si no existe
static int create_directory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) == -1) {
            perror("Error al crear el directorio");
            return -1;
        }
    }
    return 0;
}

// Función para inicializar el superbloque
static int initialize_superblock(const char *path) {
    bwfs_superblock_t sb = {
        .signature = "BWFS",
        .version = 1,
        .block_size = 1000000,  // 1000x1000 bits = 125KB
        .total_blocks = 1024,  // 1GB de almacenamiento
        .free_blocks = 1022,   // 2 bloques usados (superbloque y raíz)
        .inode_count = 1,      // Inodo raíz
        .root_inode = 1,       // Inodo raíz
        .created = time(NULL),
        .modified = time(NULL),
        .flags = 0
    };
    
    // Inicializar el array reservado con ceros
    memset(sb.reserved, 0, sizeof(sb.reserved));

    char sb_path[256];
    snprintf(sb_path, sizeof(sb_path), "%s/superblock.bin", path);
    
    FILE *fp = fopen(sb_path, "wb");
    if (!fp) {
        perror("Error al crear el superbloque");
        return -1;
    }
    
    fwrite(&sb, sizeof(bwfs_superblock_t), 1, fp);
    fclose(fp);
    return 0;
}

// Función principal
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <directorio>\n", argv[0]);
        return 1;
    }

    const char *folder = argv[1];

    // Crear directorio si no existe
    if (create_directory(folder) != 0) {
        return 1;
    }

    // Inicializar superbloque
    if (initialize_superblock(folder) != 0) {
        return 1;
    }

    printf("Sistema de archivos BWFS inicializado en: %s\n", folder);
    return 0;
}
