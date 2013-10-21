#pragma once

#include <partition.h>
#include <image.h>

#define T_UNKNOWN 0
#define T_ETX2 1

// Needs support from driver
// Read(ino)
// Write(ino)
// (ino) = Create()
// (ino, name) = Readdir(dir_ino, num)
// Link(ino, dir_ino, name)
// Unlink(dir_ino, num)
// fstat(ino)
//
// Hooks in driver:
// Load
// Close
//
//
//

typedef unsigned int INODE;

typedef struct
{
  INODE ino;
  char *name;
} dirent_t;

struct fs_st;

typedef struct
{
  int (*read)(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset);
  int (*write)(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset);
  INODE (*touch)(struct fs_st *fs);
  dirent_t *(*readdir)(struct fs_st *fs, INODE dir, unsigned int num);
  void (*link)(struct fs_st *fs, INODE ino, INODE dir, const char *name);
  void (*unlink)(struct fs_st *fs, INODE dir, unsigned int num);
  int (*fstat)(struct fs_st *fs, INODE ino);
  INODE root;

  void *(*hook_load)(struct fs_st *fs);
  void *(*hook_create)(struct fs_st *fs);
  void (*hook_close)(struct fs_st *fs);
  void (*hook_check)(struct fs_st *fs);
} fs_driver_t;

typedef struct fs_st
{
  partition_t *p;
  int type;
  void *data;
  fs_driver_t driver;
} fs_t;


