#define FUSE_USE_VERSION 31
#define MKFS_BWFS

#include "bwfs_impl.c"

int create_bwfs(const char *folder, uint32_t total_blocks);

int main(int argc, char *argv[]) {
    return main_mkfs(argc, argv);
}
