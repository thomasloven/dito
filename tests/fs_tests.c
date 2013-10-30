#include "minunit.h"
#include <errno.h>
#include "../src/fs.h"
#include <dito.h>
#include <unistd.h>

char *test_fs_load()
{
  return NULL;
}

char *test_fs_find()
{
  size_t sizes[] = {10000, 0, 0, 0};
  image_t *im = image_new("tests/testimg2.img", sizes, 0);
  partition_t *p = partition_open(im, 0);
  fs_t *fs = fs_create(p, ext2);

  INODE i = fs_find(fs, "/");
  mu_assert(i == 2, "Wrong root inode for ext2");

  fs_close(fs);
  partition_close(p);
  image_close(im);
  unlink("tests/testimg2.img");

  return NULL;
}

char *all_tests() {
  mu_suite_start();
  mu_run_test(test_fs_load);
  mu_run_test(test_fs_find);
  return NULL;
}

RUN_TESTS(all_tests);
