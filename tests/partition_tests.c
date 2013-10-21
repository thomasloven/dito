#include "minunit.h"
#include <partition.h>
#include <errno.h>
#include <unistd.h>

char *test_partition_read()
{
  size_t sizes[] = {10000, 0, 0, 0};
  image_t *im = image_new("tests/testimg2.img", sizes, 0);

  partition_t *p = partition_open(im, 0);

  char buffer[512];
  char buffer2[512];
  FILE *fp = fopen("/dev/urandom",  "r");
  fread(buffer, 512, 1, fp);
  fclose(fp);

  image_writeblocks(im, buffer, p->offset + 1, 1);

  partition_readblocks(p, buffer2, 1, 1);

  mu_assert(!memcmp(buffer, buffer2, 512), "Partition read did not return correct data");

  partition_close(p);
  image_close(im);
  unlink("tests/testimg2.img");

  return NULL;
}

char *test_partition_write()
{
  size_t sizes[] = {10000, 0, 0, 0};
  image_t *im = image_new("tests/testimg2.img", sizes, 0);

  partition_t *p = partition_open(im, 0);

  char buffer[512];
  char buffer2[512];
  FILE *fp = fopen("/dev/urandom",  "r");
  fread(buffer, 512, 1, fp);
  fclose(fp);

  partition_writeblocks(p, buffer, 0, 1);

  image_readblocks(im, buffer2, p->offset, 1);

  mu_assert(!memcmp(buffer, buffer2, 512), "Read did not return same as write");

  partition_close(p);
  image_close(im);
  unlink("tests/testimg2.img");

  return NULL;
}


char *all_tests() {
  mu_suite_start();

  mu_run_test(test_partition_read);
  mu_run_test(test_partition_write);

  return NULL;
}

RUN_TESTS(all_tests);
