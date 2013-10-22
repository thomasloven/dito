#include <ext2.h>
#include <fs.h>
#include <stdlib.h>
#include <image.h>
#include <string.h>

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

int ext2_readblocks(struct fs_st *fs, void *buffer, size_t start, size_t len)
{
  size_t db_start = start*ext2_blocksize(fs)/BLOCK_SIZE;
  size_t db_len = len*ext2_blocksize(fs)/BLOCK_SIZE;

  return partition_readblocks(fs->p, buffer, db_start, db_len);
}

int ext2_read_groupblocks(struct fs_st *fs, int group, void *buffer, size_t start, size_t len)
{
  return 0;
}

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
  ext2_data_t *data = fs->data = malloc(sizeof(ext2_data_t));

  // Read superblock
  data->superblock = malloc(sizeof(ext2_superblock_t));
  partition_readblocks(fs->p, data->superblock, 2, 2);
  data->superblock_dirty = 0;

  // Calculate number of groups
  data->num_groups = data->superblock->num_inodes / data->superblock->inodes_per_group;
  if(data->superblock->num_inodes % data->superblock->inodes_per_group) data->num_groups++;

  // Calculate size of group descriptors
  size_t groups_size = data->num_groups*sizeof(ext2_groupd_t);
  size_t groups_blocks = groups_size / ext2_blocksize(fs);
  if(groups_size % ext2_blocksize(fs))
    groups_blocks++;

  // Find location of group descriptor table
  data->groups = malloc(groups_blocks*ext2_blocksize(fs));
  size_t groups_start = 1;
  if(ext2_blocksize(fs) == 1024)
    groups_start++;

  // Read group descriptor table
  ext2_readblocks(fs, data->groups, groups_start, groups_blocks);
  data->groups_dirty = 0;

  // Clear inode buffer
  memset(&data->ino_buffer, 0, sizeof(ext2_inode_t));

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

