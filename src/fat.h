#pragma once
#include <stdint.h>
#include "fs.h"

typedef struct fat_bpb_st
{
  uint8_t jmp[3];
  uint8_t identifier[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fat_count;
  uint16_t root_count;
  uint16_t total_sectors_small;
  uint8_t media_descriptor;
  uint16_t sectors_per_fat;
  uint16_t sectors_per_track;
  uint16_t num_heads;
  uint32_t hidden_sectors;
  uint32_t total_sectors_large;
  union
  {
    struct
    {
      uint8_t drive;
      uint8_t flags;
      uint8_t signature;
      uint32_t volume_id;
      uint8_t volume_label[11];
      uint8_t system_id[8];
      uint8_t unused[448];
      uint8_t boot_signature[2];
    } fat16;
    struct
    {
      uint32_t sectors_per_fat;
      uint16_t flags2;
      uint16_t fat_version;
      uint32_t root_cluster;
      uint16_t fsinfo_cluster;
      uint16_t boot_backup_cluster;
      uint8_t reserved[12];
      uint8_t drive;
      uint8_t flags;
      uint8_t signature;
      uint32_t volume_id;
      uint8_t volume_label[11];
      uint8_t system_id[8];
      uint8_t unused[420];
      uint8_t boot_signature[2];
    } fat32;
  };
}__attribute__((packed)) fat_bpb_t;

typedef struct fat_dir_st
{
  uint8_t name[11];
  uint8_t attrib;
  uint8_t reserved;
  uint8_t csec;
  uint16_t ctime;
  uint16_t cdate;
  uint16_t adate;
  uint16_t cluster_high;
  uint16_t mtime;
  uint16_t mdate;
  uint16_t cluster_low;
  uint32_t size;
}__attribute__((packed)) fat_dir_t;

#define FAT_DIR_READ_ONLY 0x01
#define FAT_DIR_HIDDEN 0x02
#define FAT_DIR_SYSTEM 0x04
#define FAT_DIR_VOLUME_ID 0x08
#define FAT_DIR_DIRECTORY 0x10
#define FAT_DIR_ARCHIVE 0x20
#define FAT_DIR_LONGNAME 0x0F

typedef struct fat_longname_st
{
  uint8_t num;
  uint8_t name1[10];
  uint8_t attrib;
  uint8_t entry_type;
  uint8_t checksum;
  uint8_t name2[12];
  uint16_t zero;
  uint8_t name3[4];
}__attribute__((packed)) fat_longname_t;

typedef struct fat_inode_st
{
  struct fat_inode_st *next;
  INODE parent;
  uint8_t type;
  uint32_t cluster;
  uint32_t size;
  uint32_t atime;
  uint32_t ctime;
  uint32_t mtime;
} fat_inode_t;

typedef struct
{
  fat_bpb_t *bpb;
  uint8_t *fat;
  fat_inode_t *inodes;
  fat_inode_t *last;
  INODE next;
} fat_data_t;

#define fat_data(fs) ((fat_data_t *)(fs)->data)
#define fat_bpb(fs) ((fat_data((fs)))->bpb)
#define fat_root_sectors(fs) (((fat_bpb(fs)->root_count * 32) + (fat_bpb(fs)->bytes_per_sector - 1)) \
  / fat_bpb(fs)->bytes_per_sector)
#define fat_first_data_sector(fs) (fat_bpb(fs)->reserved_sectors \
  + (fat_bpb(fs)->fat_count * fat_bpb(fs)->sectors_per_fat))
#define fat_fat_start(fs) (fat_bpb(fs)->reserved_sectors)
#define fat_num_sectors(fs) ((fat_bpb(fs)->total_sectors_small + fat_bpb(fs)->total_sectors_large) \
  - fat_bpb(fs)->reserved_sectors \
  - (fat_bpb(fs)->fat_count * fat_bpb(fs)->sectors_per_fat) \
  - fat_root_sectors(fs))
#define fat_num_clusters(fs) (fat_num_sectors(fs) / fat_bpb(fs)->sectors_per_cluster)
#define fat_clustersize(fs) (fat_bpb(fs)->bytes_per_sector*fat_bpb(fs)->sectors_per_cluster)

#define fat_type(fs, a, b, c) (if(fat_bits(fs)==12)(a);else if(fat_bits(fs)==16)(b); else(c);)

fs_driver_t fat_driver;

int fat_read(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset);
int fat_write(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset);
INODE fat_touch(struct fs_st *fs, fstat_t *st);
dirent_t *fat_readdir(struct fs_st *fs, INODE dir, unsigned int num);
int fat_link(struct fs_st *fs, INODE ino, INODE dir, const char *name);
int fat_unlink(struct fs_st *fs, INODE dir, unsigned int num);
fstat_t *fat_fstat(struct fs_st *fs, INODE ino);
int fat_mkdir(struct fs_st *fs, INODE parent, const char *name);
int fat_rmdir(struct fs_st *fs, INODE dir, unsigned int num);
void *fat_hook_load(struct fs_st *fs);
void *fat_hook_create(struct fs_st *fs);
void fat_hook_close(struct fs_st *fs);
int fat_hook_check(struct fs_st *fs);


int fat_bits(struct fs_st *fs);
uint32_t fat_read_fat(struct fs_st *fs, uint32_t cluster);
void fat_write_fat(struct fs_st *fs, uint32_t cluster, uint32_t set);
fat_inode_t *fat_get_inode(struct fs_st *fs, INODE ino);
uint32_t *fat_get_clusters(struct fs_st *fs, INODE ino);
char *fat_make_shortname(const char *longname);

void *fat_write_longname(void *de, const char *name);
char *fat_read_longname(void *de);
