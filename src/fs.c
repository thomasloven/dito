#include <fs.h>


fs_t *fs_load(partition_t *p, int type);
fs_t *fs_create(partition_t *p, int type);
void fs_close(fs_t *fs);
int fs_check(fs_t *fs);

int fs_read(fs_t *fs, INODE *ino, void *buffer, size_t length, size_t offset);
int fs_write(fs_t *fs, INODE *ino, void *buffer, size_t length, size_t offset);
INODE *fs_touch(fs_t *fs);
dirent_t *fs_readdir(fs_t *fs, INODE *dir, unsigned int num);
void fs_link(fs_t *fs, INODE *ino, INODE *dir, const char *name);
void fs_unlink(fs_t *fs, INODE *dir, unsigned int num);
int fs_fstat(struct fs_st *fs, INODE *ino);

INODE *fs_finddir(fs_t *fs, INODE *dir, const char *name);
INODE *fs_find(fs_t *fs, const char *path);

