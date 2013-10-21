#include <filesystem.h>

void create_fs(struct filesystem_st *fs)
{
  if(fs->create_fs)
    return fs->create_fs(fs);
  return;
}

int check_fs(struct filesystem_st *fs)
{
  if(fs->check_fs)
    return fs->check_fs(fs);
  return -1;
}

char *read_directory(struct filesystem_st *fs, const char *path, int num)
{
  if(fs->read_directory)
    return fs->read_directory(fs, path, num);
  return 0;
}

int mkdir(struct filesystem_st *fs, const char *path)
{
  if(fs->mkdir)
    return fs->mkdir(fs, path);
  return -1;
}

int rmdir(struct filesystem_st *fs, const char *path)
{
  if(fs->rmdir)
    return fs->rmdir(fs, path);
  return -1;
}

// File info
size_t read_file(struct filesystem_st *fs, const char *path, void *buffer, size_t length, size_t offset)
{
  if(fs->read_file)
    return fs->read_file(fs, path, buffer, length, offset);
  return -1;
}

size_t write_file(struct filesystem_st *fs, const char *path, void *buffer, size_t length, size_t offset)
{
  if(fs->write_file)
    return fs->write_file(fs, path, buffer, length, offset);
  return -1;
}

int rm_file(struct filesystem_st *fs, const char *path)
{
  if(fs->rm_file)
    return fs->rm_file(fs, path);
  return -1;
}

int ln_file(struct filesystem_st *fs, const char *src, const char *dst)
{
  if(fs->ln_file)
    return fs->ln_file(fs, src, dst);
  return -1;
}


