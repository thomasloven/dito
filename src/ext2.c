#include <ext2.h>
#include <fs.h>
#include <stdlib.h>
#include <image.h>
#include <string.h>
#include <stdint.h>

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

int ext2_writeblocks(struct fs_st *fs, void *buffer, size_t start, size_t len)
{
  size_t db_start = start*ext2_blocksize(fs)/BLOCK_SIZE;
  size_t db_len = len*ext2_blocksize(fs)/BLOCK_SIZE;

  return partition_writeblocks(fs->p, buffer, db_start, db_len);
}

int ext2_read_groupblocks(struct fs_st *fs, int group, void *buffer, size_t start, size_t len)
{
  ext2_data_t *data = fs->data;
  size_t offset = group*data->superblock->blocks_per_group;
  return ext2_readblocks(fs, buffer, start+offset, len);
}

int ext2_write_groupblocks(struct fs_st *fs, int group, void *buffer, size_t start, size_t len)
{
  ext2_data_t *data = fs->data;
  size_t offset = group*data->superblock->blocks_per_group;
  return ext2_writeblocks(fs, buffer, start+offset, len);
}

int ext2_read_inode(struct fs_st *fs, ext2_inode_t *buffer, int num)
{
  ext2_data_t *data = fs->data;
  int group = (num-1) / data->superblock->inodes_per_group;
  int offset =(num-1) % data->superblock->inodes_per_group;

  int inoblock = (offset*sizeof(ext2_inode_t))/ext2_blocksize(fs);
  size_t inooffset = (offset*sizeof(ext2_inode_t))%ext2_blocksize(fs);
  inoblock += data->groups[group].inode_table;

  char *buff = malloc(2*ext2_blocksize(fs));
  ext2_read_groupblocks(fs, group, buff, inoblock, 2);
  memcpy(buffer, (void *)((size_t)buff + inooffset), sizeof(ext2_inode_t));

  free(buff);

  return 0;
}

int ext2_write_inode(struct fs_st *fs, ext2_inode_t *buffer, int num)
{
  ext2_data_t *data = fs->data;
  int group = num / data->superblock->inodes_per_group;
  int offset = num % data->superblock->inodes_per_group;

  int inoblock = (offset*sizeof(ext2_inode_t))/ext2_blocksize(fs);
  size_t inooffset = (offset*sizeof(ext2_inode_t))%ext2_blocksize(fs);

  char *buff = malloc(2*ext2_blocksize(fs));
  ext2_read_groupblocks(fs, group, buff, inoblock, 2);
  memcpy((void *)((size_t)buff + inooffset), buffer, sizeof(ext2_inode_t));
  ext2_write_groupblocks(fs, group, buff, inoblock, 2);

  free(buff);

  return 0;
}

size_t ext2_read_indirect(fs_t *fs, uint32_t block, int level, uint32_t *block_list, size_t bl_index, size_t length)
{
  if(level == 0)
  {
    block_list[bl_index] = block;
    return 1;
  } else {
    size_t i = 0;
    size_t read = 0;
    uint32_t *blocks = malloc(ext2_blocksize(fs));
    ext2_readblocks(fs, blocks, block, 1);
    while(i < ext2_blocksize(fs)/sizeof(uint32_t) && bl_index < length)
    {
      size_t read2 = ext2_read_indirect(fs, blocks[i], level-1, block_list, bl_index, length);
      bl_index += read2;
      read += read2;
      i++;
    }
    free(blocks);
    return read;
  }
}

uint32_t *ext2_get_blocks(fs_t *fs, ext2_inode_t *node)
{
  int num_blocks = node->size_low/ext2_blocksize(fs) + (node->size_low%ext2_blocksize(fs) != 0);

  uint32_t *block_list = calloc(num_blocks + 1, sizeof(uint32_t));

  int i;
  for(i = 0; i < num_blocks && i < 12; i++)
    block_list[i] = node->direct[i];
  if(i < num_blocks)
    i += ext2_read_indirect(fs, node->indirect, 1, block_list, i, num_blocks);
  if(i < num_blocks)
    i += ext2_read_indirect(fs, node->dindirect, 2, block_list, i, num_blocks);
  if(i < num_blocks)
    i += ext2_read_indirect(fs, node->tindirect, 3, block_list, i, num_blocks);

  block_list[i] = 0;
  return block_list;
}

size_t ext2_read_data(fs_t *fs, ext2_inode_t *node, void *buffer, size_t length)
{
  if(length > node->size_low)
    length = node->size_low;
  uint32_t *block_list = ext2_get_blocks(fs, node);

  int i = 0;
  while(block_list[i])
  {
    size_t readlength = length;
    if(readlength > (size_t)ext2_blocksize(fs))
      readlength = ext2_blocksize(fs);
    ext2_readblocks(fs, buffer, block_list[i], 1);

    length -= readlength;
    buffer = (void *)((size_t)buffer + readlength);
    i++;
  }
  free(block_list);
  return length;
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
  ext2_inode_t *dir_ino = malloc(sizeof(ext2_inode_t));
  ext2_read_inode(fs, dir_ino, dir);

  void *data = malloc(dir_ino->size_low);
  ext2_read_data(fs, dir_ino, data, dir_ino->size_low);

  ext2_dirinfo_t *di = data;
  while(num && (size_t)di < ((size_t)data + dir_ino->size_low))
  {
    di = (ext2_dirinfo_t *)((size_t)di + di->record_length);
    num--;
  }

  dirent_t *de = malloc(sizeof(dirent_t));
  de->ino = di->inode;
  de->name = strndup(di->name, di->name_length);

  free(data);
  free(dir_ino);

  return de;
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
  data->buffer_dirty = 0;

  return 0;
}

void *ext2_hook_create(struct fs_st *fs)
{
  return 0;
}

void ext2_hook_close(struct fs_st *fs)
{
  if(!fs)
    return;
  if(!fs->data)
    return;

  ext2_data_t *data = fs->data;
  if(data->superblock_dirty)
  {
    partition_writeblocks(fs->p, data->superblock, 2, 2);
  }
  if(data->groups_dirty)
  {
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
    ext2_writeblocks(fs, data->groups, groups_start, groups_blocks);
    data->groups_dirty = 0;
  }
  if(data->buffer_dirty)
  {
    // Rewrite buffered inode
    ext2_write_inode(fs, &data->ino_buffer, data->buffer_inode);
  }

  free(data->superblock);
  free(data->groups);
  free(data);

  return;
}

int ext2_hook_check(struct fs_st *fs)
{
  return 0;
}

