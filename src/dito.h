#pragma once
#include <stdio.h>
#include <stdint.h>

typedef struct
{
  uint8_t boot_indicator;
  uint8_t start_head;
  uint8_t start_sector;
  uint8_t start_cylinder;
  uint8_t system_id;
  uint8_t end_head;
  uint8_t end_sector;
  uint8_t end_cylinder;
  uint32_t start_LBA;
  uint32_t num_sectors;
}__attribute__((packed)) MBR_entry_t;

typedef struct
{
  char *filename;
  FILE *file;
  size_t cylinders;
  size_t heads;
  size_t sectors;
  MBR_entry_t mbr[4];
  int mbr_dirty;

} image_t;

image_t *image_new(char *filename, size_t sizes[4], int boot);
image_t *image_load(char *filename);
void image_close(image_t *im);



typedef struct
{
  image_t *im;
  int partition;
  size_t offset;
  size_t length;
} partition_t;

partition_t *partition_open(image_t *im, int partition);
void partition_close(partition_t *p);



typedef unsigned int INODE;

typedef enum fs_type_e {
  unknown,
  native,
  std,
  ext2,
  fat16,
  fat32,
  sfs,
  ntfs,
  hfs
}fs_type_t;

typedef enum fs_ftype_e {
  normal,
  dir
} fs_ftype_t;

typedef struct
{
  INODE ino;
  char *name;
} dirent_t;

typedef struct
{
  size_t size;
  uint32_t mode;
  uint32_t atime;
  uint32_t ctime;
  uint32_t mtime;
}fstat_t;
#define S_FIFO 0x1000
#define S_CHR 0x2000
#define S_DIR 0x4000
#define S_BLK 0x6000
#define S_REG 0x8000
#define S_LINK 0xA000
#define S_SOCK 0xC000
#define S_RUSR 0400
#define S_WUSR 0200
#define S_XUSR 0100
#define S_RGRP 0040
#define S_WGRP 0020
#define S_XGRP 0010
#define S_ROTH 0004
#define S_WOTH 0002
#define S_XOTH 0001

struct fs_driver_st;

typedef struct fs_st
{
  partition_t *p;
  fs_type_t type;
  void *data;
  struct fs_driver_st *driver;
} fs_t;

fs_t *fs_load(partition_t *p, fs_type_t type);
fs_t *fs_create(partition_t *p, fs_type_t type);
void fs_close(fs_t *fs);
int fs_check(fs_t *fs);

int fs_read(fs_t *fs, INODE ino, void *buffer, size_t length, size_t offset);
int fs_write(fs_t *fs, INODE ino, void *buffer, size_t length, size_t offset);
INODE fs_touch(fs_t *fs, fstat_t *st);
dirent_t *fs_readdir(fs_t *fs, INODE dir, unsigned int num);
int fs_link(fs_t *fs, INODE ino, INODE dir, const char *name);
int fs_unlink(fs_t *fs, INODE dir, unsigned int num);
fstat_t *fs_fstat(struct fs_st *fs, INODE ino);
int fs_mkdir(struct fs_st *fs, INODE parent, const char *name);
int fs_rmdir(fs_t *fs, INODE dir, unsigned int num);

INODE fs_finddir(fs_t *fs, INODE dir, const char *name);
INODE fs_find(fs_t *fs, const char *path);
INODE fs_touchp(fs_t *fs, fstat_t *st, const char *path);

