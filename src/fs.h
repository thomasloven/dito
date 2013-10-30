#pragma once

#include "partition.h"
#include "image.h"
#include <dito.h>

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



typedef struct fs_driver_st
{
  int (*read)(fs_t *fs, INODE ino, void *buffer, size_t length, size_t offset);
  int (*write)(fs_t *fs, INODE ino, void *buffer, size_t length, size_t offset);
  INODE (*touch)(fs_t *fs, fstat_t *st);
  dirent_t *(*readdir)(fs_t *fs, INODE dir, unsigned int num);
  int (*link)(fs_t *fs, INODE ino, INODE dir, const char *name);
  int (*unlink)(fs_t *fs, INODE dir, unsigned int num);
  fstat_t *(*fstat)(fs_t *fs, INODE ino);
  int (*mkdir)(fs_t *fs, INODE parent, const char *name);
  INODE root;

  void *(*hook_load)(fs_t *fs);
  void *(*hook_create)(fs_t *fs);
  void (*hook_close)(fs_t *fs);
  int (*hook_check)(fs_t *fs);
} fs_driver_t;

