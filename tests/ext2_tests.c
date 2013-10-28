#include "minunit.h"
#include <errno.h>
#include <ext2.h>
#include <time.h>
#include <fs.h>
#include <unistd.h>

char *test_ext2_load()
{
  image_t *im = image_load("tests/testimg.img");
  partition_t *p = partition_open(im, 0);

  fs_t *fs = fs_load(p, ext2);

  ext2_data_t *d = fs->data;
  mu_assert(d->num_groups == 3, "Wrong number of groups");

  fs_close(fs);
  partition_close(p);
  image_close(im);

  return NULL;
}

char *test_ext2_readdir()
{
  image_t *im = image_load("tests/testimg.img");
  mu_assert(im, "No image");
  partition_t *p = partition_open(im, 0);
  mu_assert(p, "No partition");
  fs_t *fs = fs_load(p, ext2);
  mu_assert(fs, "No filesystem");

  INODE i = fs_find(fs, "/");
  mu_assert(i, "No inode");
  dirent_t *de;
  de = fs_readdir(fs, i, 0);
  mu_assert(!strcmp(de->name, "."), "Directory listing is wrong");
  free(de->name);
  free(de);
  de = fs_readdir(fs, i, 1);
  mu_assert(!strcmp(de->name, ".."), "Directory listing is wrong");
  free(de->name);
  free(de);
  de = fs_readdir(fs, i, 2);
  mu_assert(!strcmp(de->name, "lost+found"), "Directory listing is wrong");
  free(de->name);
  free(de);

  fs_close(fs);
  partition_close(p);
  image_close(im);
  return NULL;
}

char *test_ext2_read()
{
  image_t *im = image_load("tests/testimg.img");
  partition_t *p = partition_open(im, 0);
  fs_t *fs = fs_load(p, ext2);

  INODE i = fs_find(fs, "/");

  ext2_dirinfo_t *buffer = malloc(1024);
  fs_read(fs, i, buffer, 1024, 0);
  mu_assert(!strcmp(buffer->name, "."), "Read wrong directory listing.");
  free(buffer);

  buffer = malloc(20);
  fs_read(fs, i, buffer, 20, 12);
  mu_assert(!strcmp(buffer->name, ".."), "Read with wrong offset.");
  free(buffer);

  fs_close(fs);
  partition_close(p);
  image_close(im);
  return NULL;
}

char *test_ext2_touch()
{
  system("cp tests/testimg.img tests/testimg2.img");
  image_t *im = image_load("tests/testimg2.img");
  partition_t *p = partition_open(im, 0);
  fs_t *fs = fs_load(p, ext2);

  fstat_t st =
  {
    1024*15,
    S_REG | 0777,
    time(0),
    time(0),
    time(0)
  };

  INODE i = fs_touch(fs, &st);
  mu_assert(i==13, "Wrong inode number");
  INODE j = fs_touch(fs, &st);
  mu_assert(j==14, "Wrong inode number second time");
  ext2_inode_t ino;
  ext2_read_inode(fs, &ino, i);
  
  uint32_t *blcks = ext2_get_blocks(fs, &ino);
  int k = 0;
  while(blcks[k])
  {
    k++;
  }
  free(blcks);
  mu_assert(k == 15, "Wrong number of blocks");

  fs_close(fs);
  partition_close(p);
  image_close(im);
  return NULL;
}


char *all_tests() {
  mu_suite_start();
  mu_run_test(test_ext2_load);
  mu_run_test(test_ext2_readdir);
  mu_run_test(test_ext2_read);
  mu_run_test(test_ext2_touch);
  return NULL;
}

RUN_TESTS(all_tests);
