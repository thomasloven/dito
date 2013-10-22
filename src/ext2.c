#include <ext2.h>
#include <fs.h>

fs_driver_t ext2_driver = {
  ext2_read,
  ext2_write,
  ext2_touch,
  ext2_readdir,
  ext2_link,
  ext2_unlink,
  ext2_fstat,
  2,
  ext2_hook_load,
  ext2_hook_create,
  ext2_hook_close,
  ext2_hook_check
};

int ext2_read(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset)
{
  return 0;
}

int ext2_write(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset)
{
  return 0;
}

INODE ext2_touch(struct fs_st *fs, fs_ftype_t type)
{
  return 0;
}

dirent_t *ext2_readdir(struct fs_st *fs, INODE dir, unsigned int num)
{
  return 0;
}

void ext2_link(struct fs_st *fs, INODE ino, INODE dir, const char *name)
{
  return;
}

void ext2_unlink(struct fs_st *fs, INODE dir, unsigned int num)
{
  return;
}

fstat_t *ext2_fstat(struct fs_st *fs, INODE ino)
{
  return 0;
}

void *ext2_hook_load(struct fs_st *fs)
{
  return 0;
}

void *ext2_hook_create(struct fs_st *fs)
{
  return 0;
}

void ext2_hook_close(struct fs_st *fs)
{
  return;
}

int ext2_hook_check(struct fs_st *fs)
{
  return 0;
}

