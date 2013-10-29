#include "minunit.h"
#include <partition.h>
#include <errno.h>
#include <unistd.h>

char *test_partition_read()
{
  size_t sizes[] = {10000, 0, 0, 0};
  image_t *im = image_new("tests/testimg2.img", sizes, 0);

  partition_t *p = partition_open(im, 0);

  char buffer[1024];
  char buffer2[1024];
  FILE *fp = fopen("/dev/urandom",  "r");
  fread(buffer, 1024, 1, fp);
  fclose(fp);

  image_writeblocks(im, buffer, p->offset + 1, 2);

  partition_readblocks(p, buffer2, 1, 2);

  mu_assert(!memcmp(buffer, buffer2, 1024), "Partition read did not return correct data");

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

char *test_partition_readwrite()
{
  char buffer[2048];
  char buffer2[2048];
  memset(buffer2, 0, 2048);
  FILE *fp = fopen("/dev/urandom",  "r");
  fread(buffer, 2048, 1, fp);
  fclose(fp);

  image_t *im = image_load("tests/testimg3.img");
  mu_assert(im, "No image file");
  partition_t *p = partition_open(im, 0);
  mu_assert(p, "No partition");

  partition_writeblocks(p, buffer, 5, 2);

  partition_readblocks(p, buffer2, 5, 1);

  mu_assert(!memcmp(buffer, buffer2, 512), "Read did not return same as write");
  mu_assert(memcmp(buffer, buffer2, 1024), "Read too much");

  partition_readblocks(p, buffer2, 5, 4);
  mu_assert(memcmp(buffer, buffer2, 2048), "Wrote too much");
  
  partition_close(p);
  image_close(im);
  unlink("tests/testimg2.img");

  return NULL;
}

char *all_tests() {
  mu_suite_start();

  mu_run_test(test_partition_read);
  mu_run_test(test_partition_write);
  mu_run_test(test_partition_readwrite);

  return NULL;
}

RUN_TESTS(all_tests);
