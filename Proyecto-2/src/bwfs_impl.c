#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <stdint.h>
#include <png.h>

#define BWFS_BLOCK_SIZE 1000000  // 1000x1000 pixels
#define BWFS_MAX_FILENAME 255
#define BWFS_MAX_FILES 1000
#define BWFS_SIGNATURE "BWFS"
#define BWFS_VERSION 1

// Estructuras del sistema de archivos
typedef struct {
    char signature[4];
    uint32_t version;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint32_t inode_count;
    uint32_t root_inode;
    time_t created;
    time_t modified;
} __attribute__((packed)) bwfs_superblock_t;

typedef struct {
    uint32_t inode_num;
    uint32_t type;  // 0=file, 1=directory
    uint32_t size;
    uint32_t blocks[16];  // Bloques directos
    uint32_t indirect_block;
    time_t atime;
    time_t mtime;
    time_t ctime;
    uint32_t links;
    uint32_t uid;
    uint32_t gid;
    uint32_t mode;
} __attribute__((packed)) bwfs_inode_t;

typedef struct {
    uint32_t inode;
    char name[BWFS_MAX_FILENAME];
    uint8_t type;
} __attribute__((packed)) bwfs_dirent_t;

typedef struct {
    uint32_t next_free;
} __attribute__((packed)) bwfs_free_block_t;

// Variables globales
static char *bwfs_folder = NULL;
static bwfs_superblock_t superblock;
static bwfs_inode_t *inode_table = NULL;
static uint32_t *block_bitmap = NULL;

// Funciones utilitarias para manejo de imágenes PNG
int write_png_block(const char *filename, uint8_t *data, int width, int height);
int read_png_block(const char *filename, uint8_t *data, int *width, int *height);

// Funciones de manejo de bloques
char *get_block_filename(uint32_t block_num);
int allocate_block();
void free_block(uint32_t block_num);
int write_block(uint32_t block_num, uint8_t *data, size_t size);
int read_block(uint32_t block_num, uint8_t *data, size_t size);

// Funciones de manejo de inodos
uint32_t allocate_inode();
void free_inode(uint32_t inode_num);

// Funciones FUSE
static int bwfs_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
static int bwfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags);
static int bwfs_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static int bwfs_open(const char *path, struct fuse_file_info *fi);
static int bwfs_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi);
static int bwfs_write(const char *path, const char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi);
static int bwfs_unlink(const char *path);
static int bwfs_mkdir(const char *path, mode_t mode);
static int bwfs_rmdir(const char *path);
static int bwfs_statfs(const char *path, struct statvfs *stbuf);
static int bwfs_rename(const char *from, const char *to, unsigned int flags);
static int bwfs_access(const char *path, int mask);
static int bwfs_flush(const char *path, struct fuse_file_info *fi);
static int bwfs_fsync(const char *path, int isdatasync, struct fuse_file_info *fi);
static off_t bwfs_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi);

static struct fuse_operations bwfs_oper = {
    .getattr    = bwfs_getattr,
    .readdir    = bwfs_readdir,
    .create     = bwfs_create,
    .open       = bwfs_open,
    .read       = bwfs_read,
    .write      = bwfs_write,
    .unlink     = bwfs_unlink,
    .mkdir      = bwfs_mkdir,
    .rmdir      = bwfs_rmdir,
    .statfs     = bwfs_statfs,
    .rename     = bwfs_rename,
    .access     = bwfs_access,
    .flush      = bwfs_flush,
    .fsync      = bwfs_fsync,
    .lseek      = bwfs_lseek,
};

// Función para cargar el sistema de archivos
int load_bwfs(const char *folder);

// Función principal para mount.bwfs
int main(int argc, char *argv[]);
