#include "fs.h"
#include "ext2.h"
#include "fat.h"
#include <dito.h>
#include <stdlib.h>
#include <string.h>


fs_driver_t *supported[] = {
  0, // unknown
  0, // native
  0, // std
  &ext2_driver,
  &fat_driver, // fat
  0, // sfs
  0, // ntfs
  0, // hfs

};

fs_t *fs_load(partition_t *p, fs_type_t type)
{
  if(!p)
    return 0;
  if(type == unknown)
    return 0;
  if(!supported[type])
    return 0;

  fs_t *fs = malloc(sizeof(fs_t));
  fs->p = p;
  fs->type = type;
  fs->data = 0;
  fs->driver = supported[type];

  if(fs->driver->hook_load)
    fs->driver->hook_load(fs);

  return fs;
}

fs_t *fs_create(partition_t *p, fs_type_t type)
{
  if(!p)
    return 0;
  if(type == unknown)
    return 0;
  if(!supported[type])
    return 0;

  fs_t *fs = malloc(sizeof(fs_t));
  fs->p = p;
  fs->type = type;
  fs->data = 0;
  fs->driver = supported[type];

  if(fs->driver->hook_create)
    fs->driver->hook_create(fs);

  return fs;
}

void fs_close(fs_t *fs)
{
  if(!fs)
    return;
  if(fs->driver->hook_close)
    fs->driver->hook_close(fs);

  free(fs);
}

int fs_check(fs_t *fs)
{
  if(!fs)
    return -1;
  if(!fs->driver->hook_check)
    return -1;
  return fs->driver->hook_check(fs);
}

int fs_read(fs_t *fs, INODE ino, void *buffer, size_t length, size_t offset)
{
  if(!fs)
    return 0;
  if(!fs->driver->read)
    return 0;
  return fs->driver->read(fs, ino, buffer, length, offset);
}

int fs_write(fs_t *fs, INODE ino, void *buffer, size_t length, size_t offset)
{
  if(!fs)
    return 0;
  if(!fs->driver->write)
    return 0;
  return fs->driver->write(fs, ino, buffer, length, offset);
}

INODE fs_touch(fs_t *fs, fstat_t *st)
{
  if(!fs)
    return 0;
  if(!fs->driver->touch)
    return 0;
  return fs->driver->touch(fs, st);
}

dirent_t *fs_readdir(fs_t *fs, INODE dir, unsigned int num)
{
  if(!fs)
    return 0;
  if(!fs->driver->readdir)
    return 0;
  return fs->driver->readdir(fs, dir, num);
}

int fs_link(fs_t *fs, INODE ino, INODE dir, const char *name)
{
  if(!fs)
    return 1;
  if(fs->driver->link)
    return fs->driver->link(fs, ino, dir, name);
  return 1;
}

int fs_unlink(fs_t *fs, INODE dir, unsigned int num)
{
  if(!fs)
    return 1;
  if(fs->driver->unlink)
    return fs->driver->unlink(fs, dir, num);
  return 1;
}

fstat_t *fs_fstat(struct fs_st *fs, INODE ino)
{
  if(!fs)
    return 0;
  if(!fs->driver->fstat)
    return 0;
  return fs->driver->fstat(fs, ino);
}

int fs_mkdir(struct fs_st *fs, INODE parent, const char *name)
{
  if(!fs)
    return 1;
  if(!fs->driver->mkdir)
    return 1;
  return fs->driver->mkdir(fs, parent, name);
}

int fs_rmdir(struct fs_st *fs, INODE parent, unsigned int num)
{
  if(!fs)
    return 1;
  if(fs->driver->rmdir)
    return fs->driver->rmdir(fs, parent, num);
  return 1;
}

INODE fs_finddir(fs_t *fs, INODE dir, const char *name)
{
  if(!fs)
    return 0;
  if(!dir)
    return 0;
  if(!fs->driver->readdir)
    return 0;

  int num = 0;
  dirent_t *de;
  while(1)
  {
    de = fs->driver->readdir(fs, dir, num);
    if(!de)
      return 0;
    if(!strcmp(name, de->name))
      break;
    free(de->name);
    free(de);
    num++;
  }
  INODE ret = de->ino;
  free(de->name);
  free(de);
  return ret;
}

INODE fs_find(fs_t *fs, const char *path)
{
  if(!fs)
    return 0;

  INODE current = fs->driver->root;

  char *name, *brk, *npath = strdup(path);
  for(name = strtok_r(npath, "/", &brk); \
        name; \
        name = strtok_r(0, "/", &brk))
  {
    current = fs_finddir(fs, current, name);
    if(!current)
      break;
  }
  free(npath);
  return current;
}

INODE fs_touchp(fs_t *fs, fstat_t *st, const char *path)
{
  INODE ret = fs_touch(fs, st);
  char *dir = strdup(path);
  char *de = strrchr(dir, '/');
  de[0] = '\0';
  INODE dir_ino = fs_find(fs, dir);
  if(!dir_ino)
  {
    free(dir);
    return 0;
  }
  if(fs_link(fs, ret, dir_ino, &de[1]))
  {
    free(dir);
    return 0;
  }
  free(dir);
  return ret;
}
