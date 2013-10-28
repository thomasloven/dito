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

char *test_ext2_readwrite()
{
  char buffer[4096];
  char buffer2[4096];
  memset(buffer2, 0, 4096);
  FILE *fp = fopen("/dev/urandom",  "r");
  fread(buffer, 4096, 1, fp);
  fclose(fp);

  image_t *im = image_load("tests/testimg3.img");
  mu_assert(im, "No image file");
  partition_t *p = partition_open(im, 0);
  mu_assert(p, "No partition");
  fs_t *fs = fs_load(p, ext2);
  mu_assert(fs, "No filesystem");

  ext2_writeblocks(fs, buffer, 5, 2);
  ext2_readblocks(fs, buffer2, 5, 1);
  mu_assert(!memcmp(buffer, buffer2, 1024), "Read did not return same as write");
  mu_assert(memcmp(buffer, buffer2, 2048), "Read too much");

  ext2_readblocks(fs, buffer2, 5, 4);
  mu_assert(memcmp(buffer, buffer2, 4096), "Wrote too much");

  fs_close(fs);
  partition_close(p);
  image_close(im);
  return NULL;
}

char *test_ext2_readdir()
{
  image_t *im = image_load("tests/testimg.img");
  partition_t *p = partition_open(im, 0);
  fs_t *fs = fs_load(p, ext2);

  INODE i = fs_find(fs, "/");
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

char *test_ext2_write()
{
  /* unlink("tests/testimg2.img"); */
  /* int ret = execl("/bin/cp", "/bin/cp", "tests/testimg.img", "tests/testimg2.img", NULL); */
  /* printf("Returned:%d, %s\n", ret, strerror(errno)); */
  /* printf("Returned:%d\n", ret); */
  image_t *im = image_load("tests/testimg3.img");
  partition_t *p = partition_open(im, 0);
  fs_t *fs = fs_load(p, ext2);

  fstat_t st =
  {
    2048,
    S_REG | 0777,
    time(0),
    time(0),
    time(0)
  };

  INODE i = fs_touch(fs, &st);
  printf("Inode number %d\n", i);
  INODE j = fs_touch(fs, &st);
  printf("Inode number %d\n", j);
  /* ext2_inode_t *ino = 0; */
  /* ext2_read_inode(fs, ino, i); */
  /* uint32_t *blcks = ext2_get_blocks(fs, ino); */
  /* int j = 0; */
  /* while(blcks[j] && j < 3) */
  /* { */
    /* printf("%d->\n", blcks[j]); */
    /* j++; */
  /* } */

  fs_close(fs);
  partition_close(p);
  image_close(im);
  /* unlink("tests/testimg2.img"); */
  return NULL;
}


char *all_tests() {
  mu_suite_start();
  /* mu_run_test(test_ext2_load); */
  mu_run_test(test_ext2_readwrite);
  /* mu_run_test(test_ext2_readdir); */
  /* mu_run_test(test_ext2_read); */
  mu_run_test(test_ext2_write);
  return NULL;
}

RUN_TESTS(all_tests);
