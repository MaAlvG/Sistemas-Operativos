#define FUSE_USE_VERSION 31
#define FSCK_BWFS

#include "bwfs_impl.c"

int check_bwfs_consistency(const char *folder);

int main(int argc, char *argv[]) {
    return main_fsck(argc, argv);
}
