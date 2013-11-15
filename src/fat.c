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
    return 12; // FAT12
  } else {
    if(fat_num_clusters(fs) < 65525)
    {
      return 16; // FAT16
    } else {
      return 32; // FAT32
    }
  }
}

uint32_t fat_root_cluster(struct fs_st *fs)
{
  fat_bpb_t *bpb = fat_bpb(fs);
  if(fat_bits(fs) == 32)
    return fat_bpb(fs)->fat32.root_cluster;
  return (bpb->reserved_sectors + bpb->fat_count*bpb->sectors_per_fat)/bpb->sectors_per_cluster;
  /* return fat_first_data_sector(fs) / fat_bpb(fs)->sectors_per_cluster; */
}

size_t fat_readclusters(struct fs_st *fs, void *buffer, size_t cluster, size_t length)
{
  size_t start = fat_first_data_sector(fs);
  if(cluster >= 2)
  {
    start += fat_root_sectors(fs);
    start += (cluster-2)*fat_bpb(fs)->sectors_per_cluster;
  }
  length *= fat_bpb(fs)->sectors_per_cluster;

  return partition_readblocks(fs->p, buffer, start, length);
}

size_t fat_writeclusters(struct fs_st *fs, void *buffer, size_t cluster, size_t length)
{
  size_t start = fat_first_data_sector(fs);
  if(cluster >= 2)
  {
    start += fat_root_sectors(fs);
    start += (cluster-2)*fat_bpb(fs)->sectors_per_cluster;
  }
  length *= fat_bpb(fs)->sectors_per_cluster;

  return partition_writeblocks(fs->p, buffer, start, length);
}

uint32_t fat_read_fat(struct fs_st *fs, uint32_t cluster)
{
  // FAT12
  uint32_t offset = cluster + cluster/2;
  uint16_t value = *(uint16_t *)&(fat_data(fs)->fat[offset]);
  if (cluster & 0x0001)
    value >>= 4;
  else
    value &= 0x0FFF;
  return value;
}

void fat_write_fat(struct fs_st *fs, uint32_t cluster, uint32_t set)
{
  uint32_t offset = cluster + cluster/2;
  uint16_t value = *(uint16_t *)&(fat_data(fs)->fat[offset]);
  if(cluster & 0x0001)
    value = (value & 0x000F) | (set << 4);
  else
    value = (value & 0xF000) | (set & 0x0FFF);
  *(uint16_t *)&(fat_data(fs)->fat[offset]) = value;
}

uint32_t fat_find_free(struct fs_st *fs, uint32_t offset)
{
  uint32_t i = 3;
  while(i < fat_num_clusters(fs))
  {
    if(fat_read_fat(fs, i))
      i++;
    else
      return i;
  }

  return 0;

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

uint32_t *fat_get_clusters(struct fs_st *fs, INODE ino)
{
  uint32_t c_count = 0;
  fat_inode_t *inode = fat_get_inode(fs, ino);
  uint32_t cluster = inode->cluster;
  while(cluster < 0xFF8)
  {
    c_count++;
    cluster = fat_read_fat(fs, cluster);
  }
  uint32_t *clusters = calloc(c_count+1, sizeof(uint32_t));
  uint32_t i = 0;
  cluster = inode->cluster;
  while(i < c_count)
  {
    clusters[i] = cluster;
    cluster = fat_read_fat(fs, cluster);
    i++;
  }
  return clusters;
}

uint32_t fat_get_clusternum(struct fs_st *fs, INODE ino)
{
  uint32_t ret = 0;
  fat_inode_t *inode = fat_get_inode(fs, ino);
  uint32_t cluster = inode->cluster;
  while(cluster < 0xFF8)
  {
    ret++;
    cluster = fat_read_fat(fs, cluster);
  }
  return ret;
}

char *fat_read_longname(void *de)
{
  fat_longname_t *ln = de;
  if(ln[0].attrib != FAT_DIR_LONGNAME)
    return 0;
  if(!(ln[0].num & 0x40))
    return 0;

  int entries = ln[0].num & 0x1F;
  char *name = calloc(entries*13 + 1, 1);
  int i = 0;
  int j = entries;
  while(j)
  {
    j--;
    name[i++] = ln[j].name1[0];
    name[i++] = ln[j].name1[2];
    name[i++] = ln[j].name1[4];
    name[i++] = ln[j].name1[6];
    name[i++] = ln[j].name1[8];
    name[i++] = ln[j].name2[0];
    name[i++] = ln[j].name2[2];
    name[i++] = ln[j].name2[4];
    name[i++] = ln[j].name2[6];
    name[i++] = ln[j].name2[8];
    name[i++] = ln[j].name2[10];
    name[i++] = ln[j].name3[0];
    name[i++] = ln[j].name3[2];
  }

  return name;

}

uint8_t fat_checksum(const char *shortname)
{
  int i;
  uint8_t sum = 0;
  for(i = 11; i; i--)
    sum = ((sum & 1) << 7) + (sum >> 1) + *shortname++;

  return sum;
}

char *fat_make_shortname(const char *longname)
{
  char *shortname = calloc(11, 1);
  strncpy(shortname, longname, 8);
  int i = 0;
  if(strchr(shortname, '.'))
  {
    int i = (size_t)strchr(shortname, '.') - (size_t)shortname;
    while(i < 8)
      shortname[i++] = ' ';
  }

  i = strlen(shortname);
  while(i < 8)
    shortname[i++] = ' ';
  char *dot = strrchr(longname, '.');

  strncpy(&shortname[8], &dot[1], 3);
  i = strlen(shortname);
  while(i < 11)
    shortname[i++] = ' ';
  return shortname;
}

void *fat_write_longname(void *de, const char *name)
{
  char *shortname = fat_make_shortname(name);

  size_t entries = strlen(name)/13;
  if(strlen(name)%13) entries++;
  char *buffer = calloc(entries, 26);
  size_t i = 0, j = 0;
  while(i < strlen(name))
  {
    buffer[j] = name[i];
    i++;
    j += 2;
  }
  j += 2;
  while(j < entries*26)
  {
    buffer[j++] = '\xff';
  }

  fat_longname_t *ln = de;

  i = entries;
  j = 1;
  int k = 0;
  while(i)
  {
    i--;
    ln[i].attrib = FAT_DIR_LONGNAME;
    ln[i].num = j++;
    memcpy(ln[i].name1, &buffer[k], 10);
    k += 10;
    memcpy(ln[i].name2, &buffer[k], 12);
    k += 12;
    memcpy(ln[i].name3, &buffer[k], 4);
    k += 4;
    ln[i].entry_type = 0;
    ln[i].checksum = fat_checksum(shortname);
  }
  ln[0].num |= 0x40;

  return &ln[entries];
}



int fat_read(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset)
{
  fat_inode_t *inode = fat_get_inode(fs, ino);
  uint32_t *clusters = 0;
  if(ino == 1)
  { // Root directory special case

    uint32_t dir_size = fat_bpb(fs)->root_count*32/fat_clustersize(fs);
    clusters = calloc(dir_size, sizeof(uint32_t));
    uint32_t i = 0;
    while(i < dir_size)
    {
      clusters[i] = i;
      i++;
    }
    dir_size = fat_bpb(fs)->root_count*4;
    if(offset + length > dir_size)
    {
      length = dir_size - offset;
    }

  } else {
    clusters = fat_get_clusters(fs, ino);

    if(inode->size == 0)
    {
      uint32_t size = fat_get_clusternum(fs, ino)*fat_clustersize(fs);
      if(offset + length > size)
        length = size - offset;
    } else {
      if(offset + length > inode->size)
        length = inode->size - offset;
    }
  }

  uint32_t start_cluster = offset/(fat_clustersize(fs));
  uint32_t cluster_offset = offset%(fat_clustersize(fs));
  uint32_t num_clusters = (length+cluster_offset)/(fat_clustersize(fs));
  if((length+cluster_offset)%(fat_clustersize(fs)))
    num_clusters++;

  void *buff = 0;
  void *b = buff = calloc(1, num_clusters*fat_clustersize(fs));
  uint32_t i = start_cluster;
  while( i < (start_cluster + num_clusters))
  {
    fat_readclusters(fs, b, clusters[i], 1);
    b = (void *)((size_t)b + fat_clustersize(fs));
    i++;
  }

  memcpy(buffer, (void *)((size_t)buff + cluster_offset), length);

  return length;
}

int fat_write(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset)
{
  fat_inode_t *inode = fat_get_inode(fs, ino);
  uint32_t *clusters = 0;

  if(ino == 1)
  { // Root directory special case

    uint32_t dir_size = fat_bpb(fs)->root_count*32/fat_clustersize(fs);
    clusters = calloc(dir_size, sizeof(uint32_t));
    uint32_t i = 0;
    while(i < dir_size)
    {
      clusters[i] = i;
      i++;
    }
    dir_size = fat_bpb(fs)->root_count*4;
    if(offset + length > dir_size)
    {
      length = dir_size - offset;
    }

  } else {
    clusters = fat_get_clusters(fs, ino);

    if(inode->size == 0)
    {
      uint32_t size = fat_get_clusternum(fs, ino)*fat_clustersize(fs);
      if(offset + length > size)
        length = size - offset;
    } else {
      if(offset + length > inode->size)
        length = inode->size - offset;
    }
  }

  uint32_t start_cluster = offset/(fat_clustersize(fs));
  uint32_t cluster_offset = offset%(fat_clustersize(fs));
  uint32_t num_clusters = (length+cluster_offset)/(fat_clustersize(fs));
  if((length+cluster_offset)%(fat_clustersize(fs)))
    num_clusters++;

  void *buff = 0;
  void *b = buff = calloc(1, num_clusters*fat_clustersize(fs));
  fat_read(fs, ino, buff, num_clusters*fat_clustersize(fs), offset - cluster_offset);
  memcpy((void *)((size_t)buff + cluster_offset), buffer, length);
  uint32_t i = start_cluster;
  while( i < (start_cluster + num_clusters))
  {
    fat_writeclusters(fs, b, clusters[i], 1);
    b = (void *)((size_t)b + fat_clustersize(fs));
    i++;
  }

  return length;
}

INODE fat_touch(struct fs_st *fs, fstat_t *st)
{
  // Create inode
  INODE retval = fat_data(fs)->next++;
  fat_inode_t *ino = calloc(1, sizeof(fat_inode_t));

  ino->parent = -1;
  if((st->mode & S_DIR) == S_DIR)
    ino->type = FAT_DIR_DIRECTORY;
  ino->atime = st->atime;
  ino->ctime = st->ctime;
  ino->mtime = st->mtime;
  ino->size = st->size;

  // Allocate clusters
  uint32_t size = ino->size - fat_clustersize(fs);
  uint32_t current = ino->cluster = fat_find_free(fs, 1);
  fat_write_fat(fs, current, 0xFF8);
  while(size >0)
  {
    fat_write_fat(fs, current, fat_find_free(fs, 1));
    current = fat_read_fat(fs, current);
    fat_write_fat(fs, current, 0xFF8);
    if(fat_clustersize(fs) > size)
      size = 0;
    else
      size -= fat_clustersize(fs);
  }

  fat_data(fs)->last->next = ino;
  fat_data(fs)->last = ino;

  return retval;
}

dirent_t *fat_readdir(struct fs_st *fs, INODE dir, unsigned int num)
{
  if(!fs)
    return 0;
  fat_inode_t *dir_ino = 0;
  if(!(dir_ino = fat_get_inode(fs, dir)))
    return 0;
  fat_bpb_t *bpb = fat_bpb(fs);
  if(dir_ino->type != FAT_DIR_DIRECTORY)
    return 0;

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
    void *buffer = 0;
    size_t max = 0;
    uint32_t size = 0;
    if(dir == 1)
    {
      // Root directory
      size = bpb->root_count*32;
    } else {
      // Other directory
      size = fat_get_clusternum(fs, dir)*fat_clustersize(fs);
      num +=2; // I want to handle . and .. myself
    }
    buffer = calloc(1, size);
    fat_read(fs, dir, buffer, size, 0);
    max = (size_t)buffer + size;

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

    char *longname = fat_read_longname(de);
    while(de->attrib == FAT_DIR_LONGNAME) de++;

    // Now de is the entry we want
    dirent_t *ret = calloc(1, sizeof(dirent_t));
    ret->ino = fat_data(fs)->next;
    if(longname)
    {
      ret->name = strdup(longname);
      free(longname);
    } else {
      ret->name = calloc(8, 1);
      char *c;
      if( (c = strchr((char *)de->name, ' ')))
        c[0] = '\0';
      c = stpncpy(ret->name, (char *)de->name, 8);
      if(de->attrib != FAT_DIR_DIRECTORY)
      {
        c[0] = '.';
        strncpy(&c[1], (char *)&de->name[8], 3);
        if((c = strchr(ret->name, ' ')))
          c[0] = '\0';
      } else {
      }
    }

    fat_inode_t *inode = calloc(1, sizeof(fat_inode_t));
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
    fat_data(fs)->last->next = inode;
    fat_data(fs)->last = inode;
    fat_data(fs)->next++;

    free(buffer);
    return ret;


  }
  return 0;
}

int fat_link(struct fs_st *fs, INODE ino, INODE dir, const char *name)
{
  fat_inode_t *dino = fat_get_inode(fs, dir);
  fat_inode_t *iino = fat_get_inode(fs, ino);
  iino->parent = dir;
  uint32_t size = fat_get_clusternum(fs, dir)*fat_clustersize(fs);
  void *buffer = calloc(1, size + fat_clustersize(fs));
  fat_read(fs, dir, buffer, size, 0);

  uint32_t entries = strlen(name)/13 + 1;
  uint32_t found = 0;
  fat_dir_t *firstfree = 0;
  fat_dir_t *de = buffer;
  int counter = 0;
  while(de->name[0] != 0)
  {
    if(de->name[0] == 0xE5)
    {
      if(!found)
        firstfree = de;
      found++;
      if(found >= entries)
      {
        de = firstfree;
        break;
      }
    } else {
      found = 0;
    }
    de++;
    counter++;
  }

  firstfree = de;
  // de is the first free entry.
  de = fat_write_longname(de, name);
  strncpy((char *)de->name, fat_make_shortname(name), 11);
  de->attrib = iino->type;
  de->csec = 0;
  time_t ctm = iino->ctime;
  struct tm *ctime = gmtime(&ctm);
  de->ctime = ((ctime->tm_hour & 0x1F) << 11) | ((ctime->tm_min &0x3F) << 5) | ((ctime->tm_sec & 0x1F));
  de->cdate = ((ctime->tm_year & 0x7F) << 9) | ((ctime->tm_mon & 0xF) << 5) | ((ctime->tm_mday & 0x1F));
  time_t atm = iino->atime;
  struct tm *atime = gmtime(&atm);
  de->adate = ((atime->tm_year & 0x7F) << 9) | ((atime->tm_mon & 0xF) << 5) | ((atime->tm_mday & 0x1F));
  time_t mtm = iino->mtime;
  struct tm *mtime = gmtime(&mtm);
  de->mtime = ((mtime->tm_hour & 0x1F) << 11) | ((mtime->tm_min &0x3F) << 5) | ((mtime->tm_sec & 0x1F));
  de->mdate = ((mtime->tm_year & 0x7F) << 9) | ((mtime->tm_mon & 0xF) << 5) | ((mtime->tm_mday & 0x1F));
  de->cluster_high = iino->cluster >> 16;
  de->cluster_low = iino->cluster & 0xFF;
  de->size = iino->size;

  firstfree = de;
  de++;
  if((size_t)de > (size_t)buffer + size)
  {
    // Increase size for directory
    uint32_t last = 0, current = dino->cluster;
    while(current < 0xFF8)
    {
      last = current;
      current = fat_read_fat(fs, current);
    }
    current = fat_find_free(fs, 1);
    fat_write_fat(fs, last, current);
    fat_write_fat(fs, current, 0xFF8);
    size += fat_clustersize(fs);
  }
  fat_write(fs, dir, buffer, size, 0);
  fat_read(fs, dir, buffer, size, 0);

  return 0;
}

int fat_unlink(struct fs_st *fs, INODE dir, unsigned int num)
{
  if(num < 2)
    return 0;

  fat_inode_t *dino = fat_get_inode(fs, dir);
  dirent_t *de = fat_readdir(fs, dir, num);
  INODE item = de->ino;
  free(de);
  fat_inode_t *iino = fat_get_inode(fs, item);
  fat_bpb_t *bpb = fat_bpb(fs);
  void *buffer = 0;
  if(dir == 1)
  {
    buffer = calloc(1, dino->size);
    size_t dir_size = bpb->root_count*32/fat_clustersize(fs);
    fat_readclusters(fs, buffer, dino->cluster, dir_size);
  } else {
  }


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
  partition_readblocks(fs->p, data->fat, data->bpb->reserved_sectors, fat_bpb(fs)->sectors_per_fat);

  // Generate root inode
  data->inodes = calloc(1, sizeof(fat_inode_t));
  data->inodes->parent = 1;
  data->inodes->type = FAT_DIR_DIRECTORY;
  data->inodes->cluster = 0;
  data->inodes->size = fat_bpb(fs)->root_count * sizeof(fat_dir_t);
  data->last = data->inodes;
  data->next = 2;

  return 0;
}

void *fat_hook_create(struct fs_st *fs)
{
  fat_data_t *data = fs->data = calloc(1, sizeof(fat_data_t));

  fat_bpb_t *bpb = data->bpb = calloc(1, sizeof(fat_bpb_t));

  uint32_t num_sectors = fs->p->length;
  uint32_t fs_size = num_sectors * BLOCK_SIZE;

  int fat_bits = 12;
  if(fs_size >= 0x80000000) // 2048 Mb
    fat_bits = 32;
  else if(fs_size >= 0x1000000) // 16 Mb
    fat_bits = 16;

  if(fat_bits != 12)
  {
    printf("Warning: Partition size requires fat 16 or 32, which are not implemented!\n");
    printf("Nothing is guaranteed from this point.\n");
  }

  printf(" Using FAT%d\n", fat_bits);

  int cluster_size = 8;
  while(fs_size >= 0x1000000) // 16 Mb
  {
    cluster_size *= 2;
    fs_size /= 2;
  }


  bpb->jmp[0] = 0xEB;
  bpb->jmp[1] = 0x3C;
  bpb->jmp[2] = 0x90;

  strcpy((char *)bpb->identifier, "mkdosfs");
  bpb->identifier[7] = ' ';

  bpb->bytes_per_sector = 512;
  bpb->sectors_per_cluster = cluster_size;
  bpb->reserved_sectors = (fat_bits == 32)?32:4;
  bpb->fat_count = 2;
  if(fat_bits != 32)
  {
    if(fs_size > 0x400000) // 4 mb (2.88 mb floppy disk - I think...)
      bpb->root_count = 512;
    else
      bpb->root_count = 240;
  } else {
    bpb->root_count = 0;
  }
  bpb->total_sectors_small = (num_sectors > 65535)?0:num_sectors;
  bpb->total_sectors_large = (num_sectors > 65535)?num_sectors:0;
  bpb->media_descriptor = (fs_size > 0x400000)?0xF8:0xF0;
  if(fat_bits != 32)
  {
    uint32_t fat_size = num_sectors/cluster_size - bpb->reserved_sectors;
    uint32_t entries_per_sector = bpb->bytes_per_sector*8/fat_bits;
    bpb->sectors_per_fat = fat_size/entries_per_sector;
    if(fat_size%entries_per_sector)
      bpb->sectors_per_fat++;
  } else {
    bpb->sectors_per_fat = 0;
  }
  bpb->sectors_per_track = 32;
  bpb->num_heads = 64;
  bpb->hidden_sectors = 0;


  partition_writeblocks(fs->p, data->bpb, 0, 1);

  return 0;
}

void fat_hook_close(struct fs_st *fs)
{
  fat_data_t *data = fat_data(fs);
  partition_writeblocks(fs->p, data->fat, data->bpb->reserved_sectors, fat_bpb(fs)->sectors_per_fat);
  return;
}

int fat_hook_check(struct fs_st *fs)
{
  return 0;
}

