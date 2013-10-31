#include "ext2.h"
#include "fs.h"
#include "image.h"
#include <dito.h>
#include <stdlib.h>
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
  ext2_mkdir,
  2,
  ext2_hook_load,
  ext2_hook_create,
  ext2_hook_close,
  ext2_hook_check
};

int ext2_readblocks(struct fs_st *fs, void *buffer, size_t start, size_t len)
{
  if(!fs)
    return 0;
  if(!buffer)
    return 0;

  size_t db_start = start*ext2_blocksize(fs)/BLOCK_SIZE;
  size_t db_len = len*ext2_blocksize(fs)/BLOCK_SIZE;

  return partition_readblocks(fs->p, buffer, db_start, db_len);
}

int ext2_writeblocks(struct fs_st *fs, void *buffer, size_t start, size_t len)
{
  if(!fs)
    return 0;
  if(!buffer)
    return 0;

  size_t db_start = start*ext2_blocksize(fs)/BLOCK_SIZE;
  size_t db_len = len*ext2_blocksize(fs)/BLOCK_SIZE;

  return partition_writeblocks(fs->p, buffer, db_start, db_len);
}

int ext2_read_groupblocks(struct fs_st *fs, int group, void *buffer, size_t start, size_t len)
{
  if(!fs)
    return 0;
  if(!buffer)
    return 0;
  if(group > (int)ext2_numgroups(fs))
    return 0;
  ext2_data_t *data = fs->data;
  if(start > data->superblock->blocks_per_group)
    return 0;
  if(start+len > data->superblock->blocks_per_group)
    return 0;

  size_t offset = group*data->superblock->blocks_per_group;
  return ext2_readblocks(fs, buffer, start+offset, len);
}

int ext2_write_groupblocks(struct fs_st *fs, int group, void *buffer, size_t start, size_t len)
{
  if(!fs)
    return 0;
  if(!buffer)
    return 0;
  if(group > (int)ext2_numgroups(fs))
    return 0;
  ext2_data_t *data = fs->data;
  if(start > data->superblock->blocks_per_group)
    return 0;
  if(start+len > data->superblock->blocks_per_group)
    return 0;

  size_t offset = group*data->superblock->blocks_per_group;
  return ext2_writeblocks(fs, buffer, start+offset, len);
}

int ext2_read_inode(struct fs_st *fs, ext2_inode_t *buffer, int num)
{
  if(!fs)
    return 0;
  if(!buffer)
    return 0;
  ext2_data_t *data = fs->data;
  if(num > (int)data->superblock->num_inodes)
    return 0;

  int group = (num-1) / data->superblock->inodes_per_group;
  int offset =(num-1) % data->superblock->inodes_per_group;

  int inoblock = (offset*sizeof(ext2_inode_t))/ext2_blocksize(fs);
  size_t inooffset = (offset*sizeof(ext2_inode_t))%ext2_blocksize(fs);
  inoblock += data->groups[group].inode_table;

  char *buff = malloc(2*ext2_blocksize(fs));
  if(!ext2_readblocks(fs, buff, inoblock, 2))
    return 0;
  memcpy(buffer, (void *)((size_t)buff + inooffset), sizeof(ext2_inode_t));

  free(buff);

  return 1;
}

int ext2_write_inode(struct fs_st *fs, ext2_inode_t *buffer, int num)
{
  if(!fs)
    return 0;
  if(!buffer)
    return 0;
  ext2_data_t *data = fs->data;
  if(num > (int)data->superblock->num_inodes)
    return 0;

  int group = (num-1) / data->superblock->inodes_per_group;
  int offset = (num-1) % data->superblock->inodes_per_group;

  int inoblock = (offset*sizeof(ext2_inode_t))/ext2_blocksize(fs);
  size_t inooffset = (offset*sizeof(ext2_inode_t))%ext2_blocksize(fs);
  inoblock += data->groups[group].inode_table;

  char *buff = malloc(2*ext2_blocksize(fs));
  if(!ext2_readblocks(fs, buff, inoblock, 2))
    return 0;
  memcpy((void *)((size_t)buff + inooffset), buffer, sizeof(ext2_inode_t));
  ext2_writeblocks(fs, buff, inoblock, 2);

  free(buff);

  return 1;
}

uint32_t ext2_alloc_block(fs_t *fs, unsigned int group)
{
  if(!fs)
    return 0;
  ext2_data_t *data = fs->data;
  if(group > ext2_numgroups(fs))
    return 0;

  // Check if preferred group is ok, or find another one
  if(!data->groups[group].unallocated_blocks)
    for(group = 0; group < ext2_numgroups(fs); group++)
      if(data->groups[group].unallocated_blocks)
        break;
  if(group == ext2_numgroups(fs))
    return 0;

  // Load block bitmap
  uint8_t *block_bitmap = malloc(ext2_blocksize(fs));
  if(!ext2_readblocks(fs, block_bitmap, data->groups[group].block_bitmap, 1))
    goto error;

  // Allocate a block
  unsigned int i = 4 + data->superblock->inodes_per_group*sizeof(ext2_inode_t)/ext2_blocksize(fs) + 1;
  while(block_bitmap[i/0x8]&(0x1<<(i&0x7)) && i < data->superblock->blocks_per_group)
    i++;
  if(i == data->superblock->blocks_per_group)
    goto error;
  block_bitmap[i/0x8] |= 0x1 << (i&0x7);
  data->groups[group].unallocated_blocks--;
  data->groups_dirty = 1;
  data->superblock->num_free_blocks--;
  data->superblock_dirty = 1;
  i++;
  i += data->superblock->blocks_per_group*group;

  // Write block bitmap back
  if(!ext2_writeblocks(fs, block_bitmap, data->groups[group].block_bitmap, 1))
    goto error;

  return i;

error:
  if(block_bitmap)
    free(block_bitmap);
  return 0;
}

size_t ext2_get_indirect(fs_t *fs, uint32_t block, int level, uint32_t *block_list, size_t bl_index, size_t length, uint32_t *indirects)
{
  if(!fs)
    return 0;
  if(level > 3)
    return 0;

  if(level == 0)
  {
    block_list[bl_index] = block;
    return 1;
  } else {
    size_t i = 0;
    size_t read = 0;
    uint32_t *blocks = malloc(ext2_blocksize(fs));
    if(!ext2_readblocks(fs, blocks, block, 1))
      return 0;
    while(i < ext2_blocksize(fs)/sizeof(uint32_t) && bl_index < length)
    {
      size_t read2 = ext2_get_indirect(fs, blocks[i], level-1, block_list, bl_index, length, indirects);
      if(read2 == 0)
        return 0;
      bl_index += read2;
      read += read2;
      i++;
    }
    if(indirects)
    {
      indirects[indirects[0]] = block;
      indirects[0]++;
    }
    free(blocks);
    return read;
  }
}

  size_t ext2_set_indirect(fs_t *fs, uint32_t *block, int level, uint32_t *block_list, size_t bl_index, int group, uint32_t *indirects)
{
  if(!fs)
    return 0;
  if(level > 3)
    return 0;

  if(level == 0)
  {
    *block = block_list[bl_index];
    return 1;
  } else {
    uint32_t *blocks = malloc(ext2_blocksize(fs));
    size_t i = 0;
    size_t set = 0;
    while(i < ext2_blocksize(fs)/sizeof(uint32_t) && block_list[bl_index])
    {
      size_t set_count = ext2_set_indirect(fs, &blocks[i], level-1, block_list, bl_index, group, indirects);
      if(set_count == 0)
        return 0;
      bl_index += set_count;
      set += set_count;
      i++;
    }
    if(indirects)
    {
      indirects[0]++;
      *block = indirects[indirects[0]];
    } else {
      *block = ext2_alloc_block(fs, group);
    }
    if(!ext2_writeblocks(fs, blocks, *block, 1))
      return 0;
    free(blocks);
    return set;
  }
}

uint32_t *ext2_get_blocks(fs_t *fs, ext2_inode_t *node, uint32_t *indirects)
{
  if(!fs)
    return 0;
  if(!node)
    return 0;

  int num_blocks = node->size_low/ext2_blocksize(fs) + (node->size_low%ext2_blocksize(fs) != 0);

  uint32_t *block_list = calloc(num_blocks + 1, sizeof(uint32_t));

  int i;
  for(i = 0; i < num_blocks && i < 12; i++)
    block_list[i] = node->direct[i];
  if(i < num_blocks)
    i += ext2_get_indirect(fs, node->indirect, 1, block_list, i, num_blocks, indirects);
  if(i < num_blocks)
    i += ext2_get_indirect(fs, node->dindirect, 2, block_list, i, num_blocks, indirects);
  if(i < num_blocks)
    i += ext2_get_indirect(fs, node->tindirect, 3, block_list, i, num_blocks, indirects);

  block_list[i] = 0;
  return block_list;
}

uint32_t ext2_set_blocks(fs_t *fs, ext2_inode_t *node, uint32_t *blocks, int group, uint32_t *indirects)
{
  if(!fs)
    return 0;
  if(!node)
    return 0;
  int i = 0;
  for(i = 0; blocks[i] && i < 12; i++)
  {
    node->direct[i] = blocks[i];
  }
  if(blocks[i])
    i += ext2_set_indirect(fs, &node->indirect, 1, blocks, i, group, indirects);
  if(blocks[i])
    i += ext2_set_indirect(fs, &node->dindirect, 2, blocks, i, group, indirects);
  if(blocks[i])
    i += ext2_set_indirect(fs, &node->tindirect, 3, blocks, i, group, indirects);

  if(blocks[i])
    return 0;
  return i;
}

uint32_t *ext2_make_blocks(fs_t *fs, ext2_inode_t *node, int group)
{
  if(!fs)
    return 0;
  if(!node)
    return 0;
  size_t blocks_needed = node->size_low / ext2_blocksize(fs);
  if(node->size_low % ext2_blocksize(fs)) blocks_needed++;

  uint32_t *block_list = calloc(blocks_needed + 1, sizeof(uint32_t));
  unsigned int i;
  for(i = 0; i < blocks_needed && i < 12; i++)
  {
    node->direct[i] = ext2_alloc_block(fs, group);
    block_list[i] = node->direct[i];
    if(!node->direct[i])
      goto error;
  }

error:
  if(block_list)
    free(block_list);
  return 0;
}

size_t ext2_read_data(fs_t *fs, ext2_inode_t *node, void *buffer, size_t length)
{
  if(!fs)
    return 0;
  if(!node)
    return 0;
  if(!buffer)
    return 0;

  if(length > node->size_low)
    length = node->size_low;
  uint32_t *block_list = ext2_get_blocks(fs, node, 0);

  int i = 0;
  size_t readcount = 0;
  while(block_list[i])
  {
    size_t readlength = length;
    if(readlength > (size_t)ext2_blocksize(fs))
      readlength = ext2_blocksize(fs);
    ext2_readblocks(fs, buffer, block_list[i], 1);

    length -= readlength;
    readcount += readlength;
    buffer = (void *)((size_t)buffer + readlength);
    i++;
  }
  free(block_list);
  return readcount;
}


int ext2_read(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset)
{
  if(!fs)
    return 0;
  if(ino < 2)
    return 0;
  if(!buffer)
    return 0;

  uint32_t *block_list = 0;
  void *buff = 0;
  ext2_inode_t *inode = malloc(sizeof(ext2_inode_t));
  if(!ext2_read_inode(fs, inode, ino))
    goto error;
  if(offset > inode->size_low)
    goto error;
  if(offset + length > inode->size_low)
    length = inode->size_low - offset;


  uint32_t start_block = offset/ext2_blocksize(fs);
  size_t block_offset = offset%ext2_blocksize(fs);
  int num_blocks = length/ext2_blocksize(fs);
  if((length+block_offset)%ext2_blocksize(fs))
    num_blocks++;

  
  block_list = ext2_get_blocks(fs, inode, 0);

  
  void *b = buff = malloc(num_blocks*ext2_blocksize(fs));
  unsigned int i = start_block;
  while(i < start_block + num_blocks && block_list[i])
  {
    ext2_readblocks(fs, b, block_list[i], 1);
    b = (void *)((size_t)b + ext2_blocksize(fs));
    i++;
  }
  if(i < start_block + num_blocks)
    goto error;

  memcpy(buffer, (void *)((size_t)buff + block_offset), length);

  free(buff);
  free(block_list);
  free(inode);

  return length;

error:
  if(inode)
    free(inode);
  if(block_list)
    free(block_list);
  if(buff)
    free(buff);
  return 0;
}

int ext2_write(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset)
{
  if(!fs)
    return 0;
  if(ino < 2)
    return 0;
  if(!buffer)
    return 0;

  uint32_t *block_list = 0;
  void *buff = 0;
  ext2_inode_t *inode = malloc(sizeof(ext2_inode_t));
  if(!ext2_read_inode(fs, inode, ino))
    goto error;
  if(offset > inode->size_low)
    goto error;
  if(offset + length > inode->size_low)
    length = inode->size_low - offset;

  uint32_t start_block = offset/ext2_blocksize(fs);
  size_t block_offset = offset%ext2_blocksize(fs);
  int num_blocks = length/ext2_blocksize(fs);
  if((length+block_offset)%ext2_blocksize(fs))
    num_blocks++;

  block_list = ext2_get_blocks(fs, inode, 0);

  void *b = buff = malloc(num_blocks*ext2_blocksize(fs));

  // Copy first part of first block to buffer
  // and fill the rest with input buffer data
  void *b2 = malloc(ext2_blocksize(fs));
  ext2_readblocks(fs, b2, block_list[start_block], 1);
  memcpy(buff, b2, block_offset);
  memcpy((void *)((size_t)buff + block_offset), buffer, length);

  unsigned int i = start_block;
  // Write first block from temp buffer
  ext2_writeblocks(fs, buff, block_list[i], 1);
  i++;
  b = (void *)((size_t)b + ext2_blocksize(fs));
  // Write rest from ordinary buffer
  while(i < start_block + num_blocks && block_list[i])
  {
    ext2_writeblocks(fs, b, block_list[i], 1);
    b = (void *)((size_t)b + ext2_blocksize(fs));
    i++;
  }
  if(i < start_block + num_blocks)
      goto error;

  free(buff);
  free(b2);
  free(block_list);
  free(inode);

  return length;

error:
  if(inode)
    free(inode);
  if(block_list)
    free(block_list);
  if(buff)
    free(buff);
  return 0;
}

INODE ext2_touch(struct fs_st *fs, fstat_t *st)
{
  // Sanity check
  if(!fs)
    return 0;
  uint32_t *blocks = 0;
  ext2_data_t *data = fs->data;
  if(!data->superblock->num_free_inodes)
    return 0;
  size_t blocks_needed = st->size / ext2_blocksize(fs);
  if(st->size % ext2_blocksize(fs)) blocks_needed++;

  if(data->superblock->num_free_blocks < blocks_needed)
    return 0;

  // Find suitable group
  int group = -1;
  unsigned int i = 0;
  for(i = 0; i < data->num_groups; i++)
  {
    if(data->groups[i].unallocated_inodes)
    {
      if(data->groups[i].unallocated_blocks >= blocks_needed)
      {
        group = i;
        break;
      }
    }
  }
  if(group == -1)
  {
    for(i = 0; i < data->num_groups; i++)
    {
      if(data->groups[i].unallocated_inodes)
      {
        group = i;
        break;
      }
    }
  }
  if(group == -1)
    return 0;


  // Allocate inode
  uint8_t *inode_bitmap = malloc(ext2_blocksize(fs));
  if(!ext2_readblocks(fs, inode_bitmap, data->groups[group].inode_bitmap, 1))
    goto error;
  uint32_t ino_num = 0;
  if(group == 0)
    ino_num = data->superblock->first_inode;
  while(ino_num < data->superblock->inodes_per_group)
  {
    if(!(inode_bitmap[ino_num/0x8]&(0x1<<(ino_num&0x7))))
      break;
    ino_num++;
  }
  if(ino_num == data->superblock->inodes_per_group)
    goto error;

  inode_bitmap[ino_num/0x8] |= (0x1<<(ino_num&0x7));
  data->groups[i].unallocated_inodes--;
  data->groups_dirty = 1;

  ino_num += data->superblock->inodes_per_group*group;
  ino_num++; // Inodes start at 1

  // Set up inode
  ext2_inode_t *ino = malloc(sizeof(ext2_inode_t));
  ext2_read_inode(fs, ino, ino_num);

  ino->type = 0;
  if((st->mode & S_FIFO) == S_FIFO)
    ino->type = EXT2_FIFO;
  if((st->mode & S_CHR) == S_CHR)
    ino->type = EXT2_CHDEV;
  if((st->mode & S_DIR) == S_DIR)
    ino->type = EXT2_DIR;
  if((st->mode & S_BLK) == S_BLK)
    ino->type = EXT2_BDEV;
  if((st->mode & S_REG) == S_REG)
    ino->type = EXT2_REGULAR;
  if((st->mode & S_LINK) == S_LINK)
    ino->type = EXT2_SYMLINK;
  if((st->mode & S_SOCK) == S_SOCK)
    ino->type = EXT2_SOCKET;
  ino->type |= st->mode & 0777;
  ino->uid = 0;
  ino->size_low = st->size;
  ino->atime = st->atime;
  ino->ctime = st->ctime;
  ino->mtime = st->mtime;
  ino->gid = 0;
  ino->link_count = 0;
  ino->disk_sectors = (blocks_needed+1)*ext2_blocksize(fs)/BLOCK_SIZE;
  ino->flags = 0;
  ino->osval1 = 0;
  ino->indirect = 0;
  ino->dindirect = 0;
  ino->tindirect = 0;
  ino->generation = 0;
  ino->extended_attributes = 0;
  ino->size_high = 0;
  for(i = 0; i < 12; i++)
  {
    ino->direct[i] = 0;
    ino->osval2[i] = 0;
  }

  // Allocate blocks
  blocks = malloc(sizeof(uint32_t)*(blocks_needed + 1));
  for(i = 0; i < blocks_needed; i++)
  {
    blocks[i] = ext2_alloc_block(fs, group);
  }
  blocks[i] = 0;
  if(ext2_set_blocks(fs, ino, blocks, group, 0) != blocks_needed)
    goto error;

  //Write everything
  ext2_writeblocks(fs, inode_bitmap, data->groups[group].inode_bitmap, 1);
  ext2_write_inode(fs, ino, ino_num);
  return ino_num;

error:
  if(inode_bitmap)
    free(inode_bitmap);
  if(blocks)
    free(blocks);
  return 0;
}

dirent_t *ext2_readdir(struct fs_st *fs, INODE dir, unsigned int num)
{
  if(!fs)
    return 0;
  if(dir < 2)
    return 0;

  ext2_inode_t *dir_ino = malloc(sizeof(ext2_inode_t));
  if(!ext2_read_inode(fs, dir_ino, dir))
    return 0;

  void *data = malloc(dir_ino->size_low);
  if(!ext2_read_data(fs, dir_ino, data, dir_ino->size_low))
    return 0;

  ext2_dirinfo_t *di = data;
  while(num && (size_t)di < ((size_t)data + dir_ino->size_low))
  {
    di = (ext2_dirinfo_t *)((size_t)di + di->record_length);
    num--;
  }
  if((size_t)di >= ((size_t)data + dir_ino->size_low))
    return 0;

  dirent_t *de = malloc(sizeof(dirent_t));
  de->ino = di->inode;
  de->name = strndup(di->name, di->name_length);

  free(data);
  free(dir_ino);

  return de;
}

int ext2_link(struct fs_st *fs, INODE ino, INODE dir, const char *name)
{
  if(!fs)
    return 1;
  if(!ino)
    return 1;
  if(!dir)
    return 1;
  if(!name)
    return 1;

  ext2_data_t *data = fs->data;

  ext2_inode_t *dino = malloc(sizeof(ext2_inode_t));
  if(!ext2_read_inode(fs, dino, dir))
    return 1;
  ext2_inode_t *iino = malloc(sizeof(ext2_inode_t));
  if(!ext2_read_inode(fs, iino, ino))
    return 1;
  iino->link_count++;
  ext2_write_inode(fs, iino, ino);

  ext2_dirinfo_t *di = malloc(dino->size_low + ext2_blocksize(fs));
  ext2_read(fs, dir, di, dino->size_low, 0);
  ext2_dirinfo_t *next = di, *current;
  // Find last direntry
  while((size_t)next < ((size_t)di + dino->size_low))
  {
    current = next;
    next = (void *)((size_t)current + current->record_length);
  }
  // New direntry is right after the last one
  current->record_length = ((size_t)current->name + current->name_length + 1 - (size_t)current);
  if(current->record_length < 12) current->record_length = 12;
  current->record_length += (current->record_length%4)?4-current->record_length%4:0;
  next = (void *)((size_t)current + current->record_length);

  next->inode = ino;
  strcpy(next->name, name);
  next->name_length = strlen(name);
  next->file_type = \
    (((iino->type & EXT2_FIFO) == EXT2_FIFO)?EXT2_DIR_FIFO:0) + \
    (((iino->type & EXT2_CHDEV) == EXT2_CHDEV)?EXT2_DIR_CHDEV:0) + \
    (((iino->type & EXT2_DIR) == EXT2_DIR)?EXT2_DIR_DIR:0) + \
    (((iino->type & EXT2_BDEV) == EXT2_BDEV)?EXT2_DIR_BDEV:0) + \
    (((iino->type & EXT2_REGULAR) == EXT2_REGULAR)?EXT2_DIR_REGULAR:0) + \
    (((iino->type & EXT2_SYMLINK) == EXT2_SYMLINK)?EXT2_DIR_SYMLINK:0) + \
    (((iino->type & EXT2_SOCKET) == EXT2_SOCKET)?EXT2_DIR_SOCKET:0);
  next->record_length = (size_t)next->name + next->name_length + 1 - (size_t)next;
  if(next->record_length < 12)
    next->record_length = 12;

  // Lengthen last entry
  if(((size_t)next + next->record_length - (size_t)di) > dino->size_low)
  {
    // Increase size of directory
    uint32_t *blocks = ext2_get_blocks(fs, dino, 0);
    uint32_t *blocks2 = calloc(dino->size_low/ext2_blocksize(fs) + 1, sizeof(uint32_t));
    unsigned int i = 0;
    for(i = 0; i < dino->size_low/ext2_blocksize(fs); i++)
    {
      blocks2[i] = blocks[i];
    }
    blocks2[i] = ext2_alloc_block(fs, ino/data->superblock->inodes_per_group);
    ext2_set_blocks(fs, dino, blocks2, ino/data->superblock->inodes_per_group, 0);
    free(blocks);
    free(blocks2);
    dino->size_low += ext2_blocksize(fs);
    ext2_write_inode(fs, dino, dir);
  }
  next->record_length = dino->size_low - ((size_t)next - (size_t)di);

  // Write index back
  ext2_write(fs, dir, di, dino->size_low, 0);

  free(di);
  free(iino);
  free(dino);
  
  return 0;
}

int ext2_unlink(struct fs_st *fs, INODE dir, unsigned int num)
{
  return 1;
}

fstat_t *ext2_fstat(struct fs_st *fs, INODE ino)
{
  if(!fs)
    return 0;
  if(!ino)
    return 0;
  ext2_inode_t *i = malloc(sizeof(ext2_inode_t));
  if(!ext2_read_inode(fs, i, ino))
    return 0;

  fstat_t *ret = malloc(sizeof(fstat_t));
  ret->size = i->size_low;
  ret->mode = 0;
  if((i->type & EXT2_FIFO) == EXT2_FIFO) ret->mode |= S_FIFO;
  if((i->type & EXT2_CHDEV) == EXT2_CHDEV) ret->mode |= S_CHR;
  if((i->type & EXT2_DIR) == EXT2_DIR) ret->mode |= S_DIR;
  if((i->type & EXT2_BDEV) == EXT2_BDEV) ret->mode |= S_BLK;
  if((i->type & EXT2_REGULAR) == EXT2_REGULAR) ret->mode |= S_REG;
  if((i->type & EXT2_SYMLINK) == EXT2_SYMLINK) ret->mode |= S_LINK;
  if((i->type & EXT2_SOCKET) == EXT2_SOCKET) ret->mode |= S_SOCK;

  if((i->type & EXT2_UR) == EXT2_UR) ret->mode |= S_RUSR;
  if((i->type & EXT2_UW) == EXT2_UW) ret->mode |= S_WUSR;
  if((i->type & EXT2_UX) == EXT2_UX) ret->mode |= S_XUSR;
  if((i->type & EXT2_GR) == EXT2_GR) ret->mode |= S_RGRP;
  if((i->type & EXT2_GW) == EXT2_GW) ret->mode |= S_WGRP;
  if((i->type & EXT2_GX) == EXT2_GX) ret->mode |= S_XGRP;
  if((i->type & EXT2_OR) == EXT2_OR) ret->mode |= S_ROTH;
  if((i->type & EXT2_OW) == EXT2_OW) ret->mode |= S_WOTH;
  if((i->type & EXT2_OX) == EXT2_OX) ret->mode |= S_XOTH;

  ret->atime = i->atime;
  ret->ctime = i->ctime;
  ret->mtime = i->mtime;

  free(i);
  
  return ret;
}

int ext2_mkdir(struct fs_st *fs, INODE parent, const char *name)
{
  if(!fs)
    return 1;
  if(!parent)
    return 1;
  if(!name)
    return 1;

  printf("In mkdir\n");

  fstat_t st;

  st.size = ext2_blocksize(fs);
  st.mode = S_DIR | 0755;
  st.atime = time(0);
  st.ctime = time(0);
  st.mtime = time(0);

  INODE child = fs_touch(fs, &st);
  if(fs_link(fs, child, parent, name))
  {
    return 1;
  }

  // Increase link count
  ext2_inode_t *ino = malloc(sizeof(ext2_inode_t));
  ext2_read_inode(fs, ino, child);
  ino->link_count++;
  ext2_write_inode(fs, ino, child);
  free(ino);

  // Link .
  ext2_dirinfo_t *data = calloc(1, ext2_blocksize(fs));
  data->inode = child;
  data->record_length = ext2_blocksize(fs);
  data->name_length = 1;
  data->file_type = EXT2_DIR_DIR;
  strcpy(data->name, ".");
  fs_write(fs, child, data, ext2_blocksize(fs), 0);
  free(data);

  // Link ..
  fs_link(fs, parent, child, "..");


  return 0;
}


void *ext2_hook_load(struct fs_st *fs)
{
  ext2_data_t *data = fs->data = malloc(sizeof(ext2_data_t));

  // Read superblock
  data->superblock = malloc(EXT2_SUPERBLOCK_SIZE);
  partition_readblocks(fs->p, data->superblock, 2, 2);
  data->superblock_dirty = 0;

  // Calculate number of groups
  data->num_groups = ext2_numgroups(fs);

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

