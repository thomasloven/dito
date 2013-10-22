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

typedef int fstat_t;

struct fs_st;

typedef struct
{
  int (*read)(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset);
  int (*write)(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset);
  INODE (*touch)(struct fs_st *fs, fs_ftype_t type);
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
INODE fs_touch(fs_t *fs, fs_ftype_t type);
dirent_t *fs_readdir(fs_t *fs, INODE dir, unsigned int num);
void fs_link(fs_t *fs, INODE ino, INODE dir, const char *name);
void fs_unlink(fs_t *fs, INODE dir, unsigned int num);
fstat_t *fs_fstat(struct fs_st *fs, INODE ino);

INODE fs_finddir(fs_t *fs, INODE dir, const char *name);
INODE fs_find(fs_t *fs, const char *path);

