#include "minunit.h"
#include <dito.h>
#include <errno.h>
#include "../src/ext2.h"
#include <time.h>
#include "../src/fs.h"
#include <unistd.h>

char *test_ext2_load()
{
  image_t *im = image_load("tests/testimg.img");
  mu_assert(im, "No image");
  partition_t *p = partition_open(im, 0);
  mu_assert(p, "No partition");

  fs_t *fs = fs_load(p, ext2);
  mu_assert(fs, "No filesystem");

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
  unlink("tests/testimg2.img");
  system("cp tests/testimg.img tests/testimg2.img");
  image_t *im = image_load("tests/testimg2.img");
  mu_assert(im, "No image file");
  partition_t *p = partition_open(im, 0);
  mu_assert(p, "No partition");
  fs_t *fs = fs_load(p, ext2);
  mu_assert(fs, "No file system");

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
  
  uint32_t *blcks = ext2_get_blocks(fs, &ino, 0);
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

char *test_ext2_write()
{
  unlink("tests/testimg2.img");
  system("cp tests/testimg.img tests/testimg2.img");

  char buffer[2048];
  char buffer2[2048];
  memset(buffer2, 0, 2048);
  FILE *fp = fopen("/dev/urandom",  "r");
  fread(buffer, 2048, 1, fp);
  fclose(fp);

  image_t *im = image_load("tests/testimg2.img");
  mu_assert(im, "No image file");
  partition_t *p = partition_open(im, 0);
  mu_assert(p, "No partition");
  fs_t *fs = fs_load(p, ext2);
  mu_assert(fs, "No file system");

  fstat_t st =
  {
    1024*2,
    S_REG | 0777,
    time(0),
    time(0),
    time(0)
  };

  INODE i = fs_touch(fs, &st);
  mu_assert(i, "No inode after touch");
  mu_assert(fs_write(fs, i, buffer, 1024, 512), "Write failed");
  mu_assert(fs_read(fs, i, buffer2, 2048, 0), "Read failed");

  mu_assert(!memcmp(buffer, &buffer2[512], 1024), "Wrong data read or written");


  fs_close(fs);
  partition_close(p);
  image_close(im);
  return NULL;
}

char *test_ext2_link()
{
  unlink("tests/testimg2.img");
  system("cp tests/testimg.img tests/testimg2.img");
  image_t *im = image_load("tests/testimg2.img");
  mu_assert(im, "No image file");
  partition_t *p = partition_open(im, 0);
  mu_assert(p, "No partition");
  fs_t *fs = fs_load(p, ext2);
  mu_assert(fs, "No file system");

  fstat_t st =
  {
    1024*2,
    S_REG | 0777,
    time(0),
    time(0),
    time(0)
  };

  INODE i = fs_touch(fs, &st);
  mu_assert(i, "No inode after touch");
  INODE dir = fs_find(fs, "/");

  fs_link(fs, i, dir, "TestFile");

  INODE j = fs_find(fs, "/TestFile");
  mu_assert(j == i, "Wrong inode from fs_find()");

  dirent_t *de;
  de = fs_readdir(fs, dir, 3);
  mu_assert(de->ino == i, "Wrong inode from readdir");
  mu_assert(!strcmp(de->name, "TestFile"), "Wrong name from readdir");

  fs_close(fs);
  partition_close(p);
  image_close(im);
  return NULL;
}

char *test_ext2_fstat()
{
  unlink("tests/testimg2.img");
  system("cp tests/testimg.img tests/testimg2.img");
  image_t *im = image_load("tests/testimg2.img");
  mu_assert(im, "No image file");
  partition_t *p = partition_open(im, 0);
  mu_assert(p, "No partition");
  fs_t *fs = fs_load(p, ext2);
  mu_assert(fs, "No file system");

  fstat_t st =
  {
    1024*2,
    S_REG | 0777,
    time(0),
    time(0),
    time(0)
  };

  INODE i = fs_touch(fs, &st);
  mu_assert(i, "No inode after touch");
  fstat_t *ff = fs_fstat(fs, i);
  mu_assert(ff, "No fstat returned");
  mu_assert(ff->size == 1024*2, "Wrong size");
  mu_assert(ff->mode == (S_REG | 0777), "Wrong mode");
  mu_assert(ff->atime == st.atime, "Wrong access time");
  mu_assert(ff->ctime == st.ctime, "Wrong creation time");
  mu_assert(ff->mtime == st.mtime, "Wrong modification time");
  INODE dir = fs_find(fs, "/lost+found");

  free(ff);
  ff = fs_fstat(fs, dir);
  mu_assert(ff, "No fstat returned (dir)");
  mu_assert(ff->size == 1024, "Wrong dir size");
  mu_assert(ff->mode & S_DIR, "Not a directory");
  free(ff);

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
  mu_run_test(test_ext2_write);
  mu_run_test(test_ext2_link);
  mu_run_test(test_ext2_fstat);
  return NULL;
}

RUN_TESTS(all_tests);
