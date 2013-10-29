#pragma once

#include <partition.h>
#include <image.h>

#define T_UNKNOWN 0
#define T_ETX2 1

// Needs support from driver:
// read(ino)
// write(ino)
// (ino) = touch()
// (ino, name) = readdir(dir_ino, num)
// link(ino, dir_ino, name)
// unlink(dir_ino, num)
// fstat(ino)
//
// Hooks in driver:
// Load
// Create
// Close
// Check

typedef unsigned int INODE;

typedef enum fs_type_e {
  unknown,
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

struct fs_st;

typedef struct
{
  int (*read)(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset);
  int (*write)(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset);
  INODE (*touch)(struct fs_st *fs, fstat_t *st);
  dirent_t *(*readdir)(struct fs_st *fs, INODE dir, unsigned int num);
  void (*link)(struct fs_st *fs, INODE ino, INODE dir, const char *name);
  void (*unlink)(struct fs_st *fs, INODE dir, unsigned int num);
  fstat_t *(*fstat)(struct fs_st *fs, INODE ino);
  INODE root;

  void *(*hook_load)(struct fs_st *fs);
  void *(*hook_create)(struct fs_st *fs);
  void (*hook_close)(struct fs_st *fs);
  int (*hook_check)(struct fs_st *fs);
} fs_driver_t;

typedef struct fs_st
{
  partition_t *p;
  fs_type_t type;
  void *data;
  fs_driver_t *driver;
} fs_t;

fs_t *fs_load(partition_t *p, fs_type_t type);
fs_t *fs_create(partition_t *p, fs_type_t type);
void fs_close(fs_t *fs);
int fs_check(fs_t *fs);

int fs_read(fs_t *fs, INODE ino, void *buffer, size_t length, size_t offset);
int fs_write(fs_t *fs, INODE ino, void *buffer, size_t length, size_t offset);
INODE fs_touch(fs_t *fs, fstat_t *st);
dirent_t *fs_readdir(fs_t *fs, INODE dir, unsigned int num);
void fs_link(fs_t *fs, INODE ino, INODE dir, const char *name);
void fs_unlink(fs_t *fs, INODE dir, unsigned int num);
fstat_t *fs_fstat(struct fs_st *fs, INODE ino);

INODE fs_finddir(fs_t *fs, INODE dir, const char *name);
INODE fs_find(fs_t *fs, const char *path);

