#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h> // Para dirname() y basename()
#include <time.h>

#include "bwfs.h"
#include "bwfs_image.h"

// --- Estado Global del FS ---

typedef struct {
    char *data_dir;
} bwfs_state_t;

#define BWFS_STATE ((bwfs_state_t *) fuse_get_context()->private_data)

// --- Funciones de Ayuda ---

static void get_block_path(const char *data_dir, int block_num, char *path_buf) {
    snprintf(path_buf, MAX_PATH_LEN, "%s/%d.png", data_dir, block_num);
}

static int read_metadata_block(int block_num, void *buf, size_t size) {
    char block_path[MAX_PATH_LEN];
    get_block_path(BWFS_STATE->data_dir, block_num, block_path);
    return read_data_from_image(block_path, buf, size);
}

static int write_metadata_block(int block_num, const void *buf, size_t size) {
    char block_path[MAX_PATH_LEN];
    get_block_path(BWFS_STATE->data_dir, block_num, block_path);
    return write_data_to_image(block_path, buf, size);
}

static int read_data_block(int block_num, void *buf) {
    char block_path[MAX_PATH_LEN];
    get_block_path(BWFS_STATE->data_dir, block_num, block_path);
    return read_data_from_image(block_path, buf, BLOCK_SIZE);
}

static int write_data_block(int block_num, const void *buf) {
    char block_path[MAX_PATH_LEN];
    get_block_path(BWFS_STATE->data_dir, block_num, block_path);
    return write_data_to_image(block_path, buf, BLOCK_SIZE);
}

static int find_and_set_free_bit(unsigned char *bitmap, int size_in_bytes) {
    for (int i = 0; i < size_in_bytes; i++) {
        if (bitmap[i] != 0xFF) {
            for (int j = 0; j < 8; j++) {
                if (!((bitmap[i] >> j) & 1)) {
                    int bit_num = i * 8 + j;
                    bitmap[i] |= (1 << j);
                    return bit_num;
                }
            }
        }
    }
    return -1;
}

static int path_to_inode_num(const char *path) {
    if (strcmp(path, "/") == 0) return 0;

    bwfs_inode_t inode_table[MAX_INODES];
    if (read_metadata_block(3, inode_table, sizeof(inode_table)) < 0) return -EIO;

    int current_inode_num = 0;
    char *path_copy = strdup(path);
    char *token = strtok(path_copy, "/");

    while (token != NULL) {
        bwfs_inode_t current_inode = inode_table[current_inode_num];
        if (!S_ISDIR(current_inode.mode)) {
            free(path_copy);
            return -ENOTDIR;
        }

        char data_block[BLOCK_SIZE];
        if (read_data_block(current_inode.direct_blocks[0], data_block) < 0) {
            free(path_copy);
            return -EIO;
        }

        bwfs_dir_entry_t *entries = (bwfs_dir_entry_t *)data_block;
        int max_entries = BLOCK_SIZE / sizeof(bwfs_dir_entry_t);
        int found = 0;

        for (int i = 0; i < max_entries; i++) {
            if (entries[i].name[0] != '\0' && strcmp(entries[i].name, token) == 0) {
                current_inode_num = entries[i].inode_num;
                found = 1;
                break;
            }
        }

        if (!found) {
            free(path_copy);
            return -ENOENT;
        }
        token = strtok(NULL, "/");
    }

    free(path_copy);
    return current_inode_num;
}

static int bwfs_unlink(const char *path) {
    printf("unlink en: %s\n", path);

    // 1. Obtener el i-nodo del archivo a eliminar
    int inode_num = path_to_inode_num(path);
    if (inode_num < 0) {
        printf("Error: No se pudo encontrar el inodo para %s\n", path);
        return inode_num;
    }

    // 2. Obtener el i-nodo del directorio padre
    char *path_copy = strdup(path);
    if (!path_copy) return -ENOMEM;
    
    char *file_name = basename(path_copy);
    char *parent_path = dirname(strdup(path)); // Necesitamos una copia para dirname

    printf("  - Archivo: %s, Directorio padre: %s\n", file_name, parent_path);

    int parent_inode_num = path_to_inode_num(parent_path);
    if (parent_inode_num < 0) {
        printf("Error: No se pudo encontrar el inodo del directorio padre\n");
        free(path_copy);
        free(parent_path);
        return parent_inode_num;
    }

    // Leer la tabla de i-nodos
    bwfs_inode_t inode_table[MAX_INODES];
    if (read_metadata_block(3, inode_table, sizeof(inode_table)) < 0) {
        printf("Error: No se pudo leer la tabla de inodos\n");
        free(path_copy);
        free(parent_path);
        return -EIO;
    }

    // Verificar que el padre sea un directorio
    bwfs_inode_t *parent_inode = &inode_table[parent_inode_num];
    if (!S_ISDIR(parent_inode->mode)) {
        printf("Error: El padre no es un directorio\n");
        free(path_copy);
        free(parent_path);
        return -ENOTDIR;
    }

    // Leer el bloque de datos del directorio padre
    char data_block[BLOCK_SIZE];
    if (read_data_block(parent_inode->direct_blocks[0], data_block) < 0) {
        printf("Error: No se pudo leer el bloque de datos del directorio\n");
        free(path_copy);
        free(parent_path);
        return -EIO;
    }

    // Buscar y eliminar la entrada del directorio
    bwfs_dir_entry_t *entries = (bwfs_dir_entry_t *)data_block;
    int max_entries = BLOCK_SIZE / sizeof(bwfs_dir_entry_t);
    int entry_found = 0;

    for (int i = 0; i < max_entries; i++) {
        if (entries[i].name[0] != '\0' && strcmp(entries[i].name, file_name) == 0) {
            printf("  - Eliminando entrada del directorio: %s\n", entries[i].name);
            entries[i].name[0] = '\0';
            entry_found = 1;
            break;
        }
    }

    if (!entry_found) {
        printf("Error: No se encontró la entrada del directorio\n");
        free(path_copy);
        free(parent_path);
        return -ENOENT;
    }

    // Escribir de vuelta el bloque de datos del directorio
    if (write_data_block(parent_inode->direct_blocks[0], data_block) < 0) {
        printf("Error: No se pudo escribir el bloque de datos del directorio\n");
        free(path_copy);
        free(parent_path);
        return -EIO;
    }

    // Actualizar el i-nodo del directorio padre
    parent_inode->mtime = time(NULL);
    parent_inode->ctime = time(NULL);

    // Actualizar el contador de enlaces del archivo
    bwfs_inode_t *file_inode = &inode_table[inode_num];
    file_inode->nlink--;

    printf("  - Enlaces restantes para el archivo: %lu\n", file_inode->nlink);

    // Si no hay más enlaces, liberar los bloques
    if (file_inode->nlink == 0) {
        printf("  - Liberando bloques del archivo\n");
        // Liberar bloques directos
        for (int i = 0; i < NUM_DIRECT_BLOCKS; i++) {
            if (file_inode->direct_blocks[i] != 0) {
                printf("    - Liberando bloque %u\n", file_inode->direct_blocks[i]);
                // Aquí deberías tener una función para liberar bloques en el mapa de bits
                // free_block(file_inode->direct_blocks[i]);
                file_inode->direct_blocks[i] = 0;
            }
        }
        file_inode->size = 0;
    }

    // Actualizar la tabla de inodos
    if (write_metadata_block(3, inode_table, sizeof(inode_table)) < 0) {
        printf("Error: No se pudo actualizar la tabla de inodos\n");
        free(path_copy);
        free(parent_path);
        return -EIO;
    }

    printf("  - Archivo eliminado exitosamente\n");
    free(path_copy);
    free(parent_path);
    return 0;
}

// --- Operaciones FUSE ---

static int bwfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void)fi;
    printf("getattr en: %s\n", path);
    memset(stbuf, 0, sizeof(struct stat));

    int inode_num = path_to_inode_num(path);
    if (inode_num < 0) return inode_num;

    bwfs_inode_t inode_table[MAX_INODES];
    if (read_metadata_block(3, inode_table, sizeof(inode_table)) < 0) return -EIO;

    bwfs_inode_t inode = inode_table[inode_num];
    stbuf->st_mode = inode.mode;
    stbuf->st_nlink = inode.nlink;
    stbuf->st_uid = inode.uid;
    stbuf->st_gid = inode.gid;
    stbuf->st_size = inode.size;
    stbuf->st_atime = inode.atime;
    stbuf->st_mtime = inode.mtime;
    stbuf->st_ctime = inode.ctime;

    return 0;
}

static int bwfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    (void)offset; (void)fi; (void)flags;
    printf("readdir en: %s\n", path);

    int inode_num = path_to_inode_num(path);
    if (inode_num < 0) return inode_num;

    bwfs_inode_t inode_table[MAX_INODES];
    if (read_metadata_block(3, inode_table, sizeof(inode_table)) < 0) return -EIO;

    bwfs_inode_t inode = inode_table[inode_num];
    if (!S_ISDIR(inode.mode)) return -ENOTDIR;

    char data_block[BLOCK_SIZE];
    if (read_data_block(inode.direct_blocks[0], data_block) < 0) return -EIO;

    bwfs_dir_entry_t *entries = (bwfs_dir_entry_t *)data_block;
    int max_entries = BLOCK_SIZE / sizeof(bwfs_dir_entry_t);

    for (int i = 0; i < max_entries; i++) {
        if (entries[i].name[0] != '\0') {
            filler(buf, entries[i].name, NULL, 0, 0);
        }
    }
    return 0;
}

static int bwfs_mkdir(const char *path, mode_t mode) {
    printf("mkdir en: %s\n", path);

    char *path_copy_for_names = strdup(path);
    char *new_dir_name = basename(path_copy_for_names);
    
    char *path_copy_for_parent = strdup(path);
    char *parent_path = dirname(path_copy_for_parent);

    int parent_inode_num = path_to_inode_num(parent_path);
    if (parent_inode_num < 0) {
        free(path_copy_for_names);
        free(path_copy_for_parent);
        return parent_inode_num;
    }

    unsigned char inode_bitmap[MAX_INODES / 8];
    unsigned char block_bitmap[MAX_BLOCKS / 8];
    bwfs_inode_t inode_table[MAX_INODES];
    read_metadata_block(1, inode_bitmap, sizeof(inode_bitmap));
    read_metadata_block(2, block_bitmap, sizeof(block_bitmap));
    read_metadata_block(3, inode_table, sizeof(inode_table));

    int new_inode_num = find_and_set_free_bit(inode_bitmap, sizeof(inode_bitmap));
    if (new_inode_num == -1) { free(path_copy_for_names); free(path_copy_for_parent); return -ENOSPC; }

    int new_block_num = find_and_set_free_bit(block_bitmap, sizeof(block_bitmap));
    if (new_block_num == -1) { free(path_copy_for_names); free(path_copy_for_parent); return -ENOSPC; }

    bwfs_inode_t *new_inode = &inode_table[new_inode_num];
    new_inode->mode = S_IFDIR | mode;
    new_inode->nlink = 2;
    new_inode->uid = getuid();
    new_inode->gid = getgid();
    new_inode->atime = new_inode->mtime = new_inode->ctime = time(NULL);
    new_inode->size = sizeof(bwfs_dir_entry_t) * 2;
    new_inode->direct_blocks[0] = new_block_num;

    bwfs_dir_entry_t new_dir_entries[BLOCK_SIZE / sizeof(bwfs_dir_entry_t)];
    memset(new_dir_entries, 0, sizeof(new_dir_entries));
    strcpy(new_dir_entries[0].name, ".");
    new_dir_entries[0].inode_num = new_inode_num;
    strcpy(new_dir_entries[1].name, "..");
    new_dir_entries[1].inode_num = parent_inode_num;

    bwfs_inode_t *parent_inode = &inode_table[parent_inode_num];
    char parent_data_block[BLOCK_SIZE];
    read_data_block(parent_inode->direct_blocks[0], parent_data_block);
    
    bwfs_dir_entry_t *parent_entries = (bwfs_dir_entry_t *)parent_data_block;
    int max_entries = BLOCK_SIZE / sizeof(bwfs_dir_entry_t);
    int entry_added = 0;
    for (int i = 0; i < max_entries; i++) {
        if (parent_entries[i].name[0] == '\0') {
            strcpy(parent_entries[i].name, new_dir_name);
            parent_entries[i].inode_num = new_inode_num;
            entry_added = 1;
            break;
        }
    }
    if (!entry_added) { free(path_copy_for_names); free(path_copy_for_parent); return -ENOSPC; }

    parent_inode->nlink++;

    write_metadata_block(1, inode_bitmap, sizeof(inode_bitmap));
    write_metadata_block(2, block_bitmap, sizeof(block_bitmap));
    write_metadata_block(3, inode_table, sizeof(inode_table));
    write_data_block(new_block_num, new_dir_entries);
    write_data_block(parent_inode->direct_blocks[0], parent_data_block);

    free(path_copy_for_names);
    free(path_copy_for_parent);
    return 0;
}

static int bwfs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    printf("create en: %s\n", path);

    char *path_copy_for_names = strdup(path);
    char *new_file_name = basename(path_copy_for_names);

    char *path_copy_for_parent = strdup(path);
    char *parent_path = dirname(path_copy_for_parent);

    int parent_inode_num = path_to_inode_num(parent_path);
    if (parent_inode_num < 0) {
        free(path_copy_for_names);
        free(path_copy_for_parent);
        return parent_inode_num;
    }

    unsigned char inode_bitmap[MAX_INODES / 8];
    bwfs_inode_t inode_table[MAX_INODES];
    read_metadata_block(1, inode_bitmap, sizeof(inode_bitmap));
    read_metadata_block(3, inode_table, sizeof(inode_table));

    int new_inode_num = find_and_set_free_bit(inode_bitmap, sizeof(inode_bitmap));
    if (new_inode_num == -1) { free(path_copy_for_names); free(path_copy_for_parent); return -ENOSPC; }

    bwfs_inode_t *new_inode = &inode_table[new_inode_num];
    new_inode->mode = S_IFREG | mode;
    new_inode->nlink = 1;
    new_inode->uid = getuid();
    new_inode->gid = getgid();
    new_inode->size = 0;
    new_inode->atime = new_inode->mtime = new_inode->ctime = time(NULL);

    bwfs_inode_t *parent_inode = &inode_table[parent_inode_num];
    char parent_data_block[BLOCK_SIZE];
    read_data_block(parent_inode->direct_blocks[0], parent_data_block);

    bwfs_dir_entry_t *parent_entries = (bwfs_dir_entry_t *)parent_data_block;
    int max_entries = BLOCK_SIZE / sizeof(bwfs_dir_entry_t);
    int entry_added = 0;
    for (int i = 0; i < max_entries; i++) {
        if (parent_entries[i].name[0] == '\0') {
            strcpy(parent_entries[i].name, new_file_name);
            parent_entries[i].inode_num = new_inode_num;
            entry_added = 1;
            break;
        }
    }
    if (!entry_added) { free(path_copy_for_names); free(path_copy_for_parent); return -ENOSPC; }

    write_metadata_block(1, inode_bitmap, sizeof(inode_bitmap));
    write_metadata_block(3, inode_table, sizeof(inode_table));
    write_data_block(parent_inode->direct_blocks[0], parent_data_block);

    fi->fh = new_inode_num; // Devolver el número de i-nodo como 'file handle'

    free(path_copy_for_names);
    free(path_copy_for_parent);
    return 0;
}

static int bwfs_open(const char *path, struct fuse_file_info *fi) {
    printf("open en: %s\n", path);
    int inode_num = path_to_inode_num(path);
    if (inode_num < 0) {
        return inode_num;
    }
    fi->fh = inode_num;
    return 0;
}

static int bwfs_write(const char *path, const char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    (void)path;
    printf("--- BWFS_WRITE ---\n");
    printf("write en inode %lu, size: %zu, offset: %ld\n", fi->fh, size, offset);

    uint64_t inode_num = fi->fh;

    if (offset >= BLOCK_SIZE) { // Solo soportamos un bloque por ahora
        return -EFBIG; // Archivo demasiado grande
    }

    unsigned char block_bitmap[MAX_BLOCKS / 8];
    bwfs_inode_t inode_table[MAX_INODES];
    read_metadata_block(2, block_bitmap, sizeof(block_bitmap));
    read_metadata_block(3, inode_table, sizeof(inode_table));

    bwfs_inode_t *inode = &inode_table[inode_num];

    if (S_ISDIR(inode->mode)) {
        return -EISDIR;
    }

    int block_to_write = inode->direct_blocks[0];

    // Si el archivo no tiene un bloque de datos, asígnale uno
    if (block_to_write == 0) {
        int new_block_num = find_and_set_free_bit(block_bitmap, sizeof(block_bitmap));
        if (new_block_num == -1) {
            return -ENOSPC;
        }
        inode->direct_blocks[0] = new_block_num;
        block_to_write = new_block_num;
        printf("  - WRITE: Asignado nuevo bloque %d al inodo %lu.\n", new_block_num, inode_num);
        printf("  - WRITE: Inode %lu, block[0] en memoria AHORA es %u.\n", inode_num, inode->direct_blocks[0]);
        write_metadata_block(2, block_bitmap, sizeof(block_bitmap));
        write_metadata_block(3, inode_table, sizeof(inode_table)); // Persistir el nuevo bloque asignado
    }

    // Leemos el bloque, lo modificamos en memoria y lo volvemos a escribir
    char data_block[BLOCK_SIZE];
    memset(data_block, 0, BLOCK_SIZE);
    if (inode->size > 0) {
        read_data_block(block_to_write, data_block);
    }

    size_t bytes_to_write = size;
    if (offset + size > BLOCK_SIZE) {
        bytes_to_write = BLOCK_SIZE - offset;
        fprintf(stderr, "Advertencia: escritura truncada para caber en un bloque. Se escribieron %zu bytes.\n", bytes_to_write);
    }

    memcpy(data_block + offset, buf, bytes_to_write);
    write_data_block(block_to_write, data_block);

    if (offset + bytes_to_write > inode->size) {
        inode->size = offset + bytes_to_write;
    }
    inode->mtime = time(NULL);

    printf("  - WRITE: Actualizando size a %ld. Inode %lu, block[0] en memoria AHORA es %u.\n", inode->size, inode_num, inode->direct_blocks[0]);
    write_metadata_block(3, inode_table, sizeof(inode_table));

    return bytes_to_write;
}

static int bwfs_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi)
{
    (void)path;
    printf("--- BWFS_READ ---\n");
    printf("read en inode %lu, size: %zu, offset: %ld\n", fi->fh, size, offset);

    uint64_t inode_num = fi->fh;
    bwfs_inode_t inode_table[MAX_INODES];
    read_metadata_block(3, inode_table, sizeof(inode_table));

    bwfs_inode_t *inode = &inode_table[inode_num];
    printf("  - Inode %lu: size=%ld, block[0]=%u\n", inode_num, inode->size, inode->direct_blocks[0]);

    if (S_ISDIR(inode->mode)) {
        printf("  - Error: es un directorio.\n");
        return -EISDIR;
    }

    if (offset >= inode->size) {
        printf("  - Condición EOF: offset (%ld) >= size (%ld). Devolviendo 0.\n", offset, inode->size);
        return 0; // No hay más datos que leer
    }

    int block_to_read = inode->direct_blocks[0];
    if (block_to_read == 0) {
        printf("  - Archivo vacío (sin bloque de datos). Devolviendo 0.\n");
        return 0; // Archivo vacío, no hay nada que leer
    }

    printf("  - Leyendo del bloque de datos %d\n", block_to_read);
    char data_block[BLOCK_SIZE];
    if (read_data_block(block_to_read, data_block) < 0) {
        printf("  - Error: no se pudo leer el bloque de datos.\n");
        return -EIO;
    }

    size_t bytes_to_read = size;
    if (offset + size > inode->size) {
        bytes_to_read = inode->size - offset;
    }
    printf("  - Bytes a leer: %zu\n", bytes_to_read);

    memcpy(buf, data_block + offset, bytes_to_read);

    printf("  - Lectura exitosa. Devolviendo %zu bytes.\n", bytes_to_read);
    return bytes_to_read;
}

// --- Estructura de Operaciones ---

static struct fuse_operations bwfs_oper = {
    .getattr = bwfs_getattr,
    .readdir = bwfs_readdir,
    .mkdir   = bwfs_mkdir,
    .create  = bwfs_create,
    .open    = bwfs_open,
    .write   = bwfs_write,
    .read    = bwfs_read,
    .unlink  = bwfs_unlink,
};

// --- Main ---

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <directorio_datos> <punto_montaje> [opciones FUSE]\n", argv[0]);
        return 1;
    }

    bwfs_state_t *state = malloc(sizeof(bwfs_state_t));
    if (!state) {
        perror("malloc");
        return 1;
    }

    // Guardamos la ruta al directorio de datos.
    state->data_dir = realpath(argv[1], NULL);
    if (!state->data_dir) {
        perror("realpath");
        free(state);
        return 1;
    }

    // FUSE se encargará de los argumentos de la línea de comandos que no entendemos.
    // Pasamos nuestro estado `state` a FUSE para que esté disponible en todas las operaciones.
    return fuse_main(argc - 1, argv + 1, &bwfs_oper, state);
}
