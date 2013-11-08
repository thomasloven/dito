#include "fat.h"
#include "fs.h"
#include "image.h"
#include <dito.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

fs_driver_t fat_driver = {
  fat_read,
  fat_write,
  fat_touch,
  fat_readdir,
  fat_link,
  fat_unlink,
  fat_fstat,
  fat_mkdir,
  fat_rmdir,
  1,
  fat_hook_load,
  fat_hook_create,
  fat_hook_close,
  fat_hook_check
};

int fat_bits(struct fs_st *fs)
{
  // Return values:
  // 0: FAT12
  // 1: FAT16
  // 2: FAT32

  if(fat_num_clusters(fs) < 4085)
  {
    return 0; // FAT12
  } else {
    if(fat_num_clusters(fs) < 65525)
    {
      return 1; // FAT16
    } else {
      return 2; // FAT32
    }
  }
}

uint32_t fat_root_cluster(struct fs_st *fs)
{
  fat_bpb_t *bpb = fat_bpb(fs);
  if(fat_bits(fs) == 2)
    return fat_bpb(fs)->fat32.root_cluster;
  return (bpb->reserved_sectors + bpb->fat_count*bpb->sectors_per_fat)/bpb->sectors_per_cluster;
  /* return fat_first_data_sector(fs) / fat_bpb(fs)->sectors_per_cluster; */
}


size_t fat_readclusters(struct fs_st *fs, void *buffer, size_t cluster, size_t length)
{
  size_t start = cluster*fat_bpb(fs)->sectors_per_cluster;
  length *= fat_bpb(fs)->sectors_per_cluster;

  return partition_readblocks(fs->p, buffer, start, length);
}

size_t fat_writeclusters(struct fs_st *fs, void *buffer, size_t cluster, size_t length)
{
  size_t startsector = (cluster - 2)*fat_bpb(fs)->sectors_per_cluster + fat_first_data_sector(fs);
  length *= fat_bpb(fs)->sectors_per_cluster;

  return partition_writeblocks(fs->p, buffer, startsector, length);
}

uint32_t fat_read_fat(struct fs_st *fs, uint32_t cluster)
{
  uint32_t offset = cluster + cluster/2;
  uint16_t value = *(uint16_t *)&fat_data(fs)->fat[offset];
  if (cluster & 0x0001)
    value >>= 4;
  else
    value &= 0x0FFF;
  return value;
}

fat_inode_t *fat_get_inode(struct fs_st *fs, INODE ino)
{
  ino--;
  fat_inode_t *ret = fat_data(fs)->inodes;
  while(ino)
  {
    ret = ret->next;
    ino--;
    if(!ret)
      return ret;
  }
  return ret;
}


int fat_read(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset)
{
  return 0;
}

int fat_write(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset)
{
  return 0;
}

INODE fat_touch(struct fs_st *fs, fstat_t *st)
{
  return 0;
}

dirent_t *fat_readdir(struct fs_st *fs, INODE dir, unsigned int num)
{
  if(!fs)
    return 0;
  fat_inode_t *dir_ino = fat_get_inode(fs, dir);
  fat_bpb_t *bpb = fat_bpb(fs);
  if(dir_ino->type != FAT_DIR_DIRECTORY)
  {
    return 0;
  }
  if(num == 0)
  {
    // Return .
    dirent_t *ret = calloc(1, sizeof(dirent_t));
    ret->name = strdup(".");
    ret->ino = dir;
    return ret;
  } else if(num == 1) {
    // Return ..
    dirent_t *ret = calloc(1, sizeof(dirent_t));
    ret->name = strdup("..");
    ret->ino = dir_ino->parent;
    return ret;
  } else {
    void *buffer = calloc(1, dir_ino->size);
    size_t max = (size_t)buffer;
    if(dir == 1)
    {
      // Root directory
      size_t dir_size = bpb->root_count*32/bpb->bytes_per_sector/bpb->sectors_per_cluster;
      fat_readclusters(fs, buffer, dir_ino->cluster, dir_size);
      max += bpb->root_count;
    } else {
      // Other directory
    }

    fat_dir_t *de = buffer;
    while((num > 2) && ((size_t)de < max))
    {
      if(de->name[0] == 0) // Last entry
      {
        free(buffer);
        return 0;
      }
      if(de->attrib == FAT_DIR_LONGNAME)
      {
        de++;
      } else {
        de++;
        num--;
      }
    }
    if(de->name[0] == 0)
    {
      free(buffer);
      return 0;
    }
    while(de->attrib == FAT_DIR_LONGNAME)
    {
      // Ignore long names for now
      de++;
    }

    // Now de is the entry we want
    dirent_t *ret = calloc(1, sizeof(dirent_t));
    ret->ino = fat_data(fs)->next;
    ret->name = calloc(8, 1);
    char *c;
    if( (c = strchr(de->name, ' ')))
      c[0] = '\0';
    c = stpncpy(ret->name, de->name, 8);
    if(de->attrib != FAT_DIR_DIRECTORY)
    {
      c[0] = '.';
      strncpy(&c[1], &de->name[8], 3);
    } else {
    }

    fat_inode_t *inode = fat_data(fs)->last->next = calloc(1, sizeof(fat_inode_t));
    inode->parent = dir;
    inode->type = de->attrib;
    inode->cluster = (de->cluster_high << 16) + de->cluster_low;
    inode->size = de->size;
    struct tm atime = 
    {
      0, 0, 0, //sec, min, hour
      (de->adate & 0x1F), // day
      ((de->adate >> 5) & 0xF), // month
      ((de->adate >> 9) & 0x7F), // year
      0,0,0,0,0
    };
    struct tm ctime = 
    {
      (de->ctime & 0x1F), // sec
      ((de->ctime >> 5) & 0x3F), // min
      ((de->ctime >> 11) & 0x1F), // hour
      (de->cdate & 0x1F), // day
      ((de->cdate >> 5) & 0xF), // month
      ((de->cdate >> 9) & 0x7F), // year
      0,0,0,0,0
    };
    struct tm mtime = 
    {
      (de->mtime & 0x1F), // sec
      ((de->mtime >> 5) & 0x3F), // min
      ((de->mtime >> 11) & 0x1F), // hour
      (de->mdate & 0x1F), // day
      ((de->mdate >> 5) & 0xF), // month
      ((de->mdate >> 9) & 0x7F), // year
      0,0,0,0,0
    };
    inode->atime = mktime(&atime);
    inode->ctime = mktime(&ctime);
    inode->mtime = mktime(&mtime);
    fat_data(fs)->last = inode;
    fat_data(fs)->next++;

    free(buffer);
    return ret;


  }
  return 0;
}

int fat_link(struct fs_st *fs, INODE ino, INODE dir, const char *name)
{
  return 0;
}

int fat_unlink(struct fs_st *fs, INODE dir, unsigned int num)
{
  return 0;
}

fstat_t *fat_fstat(struct fs_st *fs, INODE ino)
{
  if(!fs)
    return 0;
  fat_inode_t *inode = fat_get_inode(fs, ino);
  if(!inode)
    return 0;

  fstat_t *ret = calloc(1, sizeof(fstat_t));
  ret->size = inode->size;
  if(inode->type == FAT_DIR_DIRECTORY)
    ret->mode = S_DIR;
  ret->mode |= 0777;
  ret->atime = inode->atime;
  ret->ctime = inode->ctime;
  ret->mtime = inode->mtime;
  
  return ret;
}

int fat_mkdir(struct fs_st *fs, INODE parent, const char *name)
{
  return 0;
}

int fat_rmdir(struct fs_st *fs, INODE dir, unsigned int num)
{
  return 0;
}

void *fat_hook_load(struct fs_st *fs)
{
  fat_data_t *data = fs->data = calloc(1, sizeof(fat_data_t));

  // Read BPB
  data->bpb = calloc(1, BLOCK_SIZE);
  partition_readblocks(fs->p, data->bpb, 0, 1);

  // Read FAT
  data->fat = calloc(fat_bpb(fs)->sectors_per_fat, BLOCK_SIZE);
  partition_readblocks(fs->p, data->fat, fat_fat_start(fs), fat_bpb(fs)->sectors_per_fat);

  // Generate root inode
  data->inodes = calloc(1, sizeof(fat_inode_t));
  data->inodes->parent = 1;
  data->inodes->type = FAT_DIR_DIRECTORY;
  data->inodes->cluster = fat_root_cluster(fs);
  data->inodes->size = fat_bpb(fs)->root_count * sizeof(fat_dir_t);
  data->last = data->inodes;
  data->next = 2;

  return 0;
}

void *fat_hook_create(struct fs_st *fs)
{
  return 0;
}

void fat_hook_close(struct fs_st *fs)
{
  return;
}

int fat_hook_check(struct fs_st *fs)
{
  return 0;
}

