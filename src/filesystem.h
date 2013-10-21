#pragma once

#include <partition.h>
#include <image.h>

#define T_UNKNOWN 0
#define T_ETX2 1

typedef struct filesystem_st
{
  partition_t *p;
  int type;
  void *data;
  void (*create_fs)(struct filesystem_st *fs);
  int (*check_fs)(struct filesystem_st *fs);
  char *(*read_directory)(struct filesystem_st *fs, const char *path, int num);
  int (*mkdir)(struct filesystem_st *fs, const char *path);
  int (*rmdir)(struct filesystem_st *fs, const char *path);
  // File info
  size_t (*read_file)(struct filesystem_st *fs, const char *path, void *buffer, size_t length, size_t offset);
  size_t (*write_file)(struct filesystem_st *fs, const char *path, void *buffer, size_t length, size_t offset);
  int (*rm_file)(struct filesystem_st *fs, const char *path);
  int (*ln_file)(struct filesystem_st *fs, const char *src, const char *dst);
} filesystem_t;

void create_fs(struct filesystem_st *fs);
int check_fs(struct filesystem_st *fs);
char *read_directory(struct filesystem_st *fs, const char *path, int num);
int mkdir(struct filesystem_st *fs, const char *path);
int rmdir(struct filesystem_st *fs, const char *path);
// File info
size_t read_file(struct filesystem_st *fs, const char *path, void *buffer, size_t length, size_t offset);
size_t write_file(struct filesystem_st *fs, const char *path, void *buffer, size_t length, size_t offset);
int rm_file(struct filesystem_st *fs, const char *path);
int ln_file(struct filesystem_st *fs, const char *src, const char *dst);


