#include "minunit.h"
#include "../src/image.h"
#include <dito.h>
#include <errno.h>
#include <unistd.h>

char *test_image_load()
{
  image_t *im = image_load("tests/testimg.img");

  mu_assert(im != NULL, "Did not return image");
  mu_assert(im->file != NULL, "Did not open a file");

  mu_assert(im->cylinders == 0x147, "Test image has wrong number of cylinders");
  mu_assert(im->heads == 0x2, "Test image has wrong number of heads");
  mu_assert(im->sectors == 0x3f, "Test image has wrong number of sectors");

  image_close(im);

  return NULL;
}

char *test_image_new()
{
  size_t sizes[] = {10000000, 5000000, 0, 5000000};
  image_t *im = image_new("tests/testimg2.img", sizes, 0);

  mu_assert(im != NULL, "Did not return image");
  mu_assert(im->file != NULL, "Did not open a file");

  mu_assert(image_getsize(im) > (sizes[0]+sizes[1]+sizes[2]+sizes[3]), "Image size is too small");
  mu_assert(image_getsize(im) < 2*(sizes[0]+sizes[1]+sizes[2]+sizes[3]), "Image size seems much too large");

  mu_assert(!image_check(im), "Image check failed");

  image_close(im);

  im = image_load("tests/testimg2.img");
  mu_assert(!image_check(im), "Second image check failed");

  image_close(im);
  unlink("tests/testimg2.img");

  return NULL;
}

char *test_image_readwrite()
{
  char buffer[2048];
  char buffer2[2048];
  memset(buffer2, 0, 2048);
  FILE *fp = fopen("/dev/urandom",  "r");
  fread(buffer, 2048, 1, fp);
  fclose(fp);

  size_t sizes[] = {10000, 0, 0, 0};
  image_t *im = image_new("tests/testimg2.img", sizes, 0);
  image_writeblocks(im, buffer, 5, 2);

  image_close(im);
  im = image_load("tests/testimg2.img");
  image_readblocks(im, buffer2, 5, 1);

  mu_assert(!memcmp(buffer, buffer2, 512), "Read did not return same as write");
  mu_assert(memcmp(buffer, buffer2, 1024), "Read too much");

  image_readblocks(im, buffer2, 5, 4);
  mu_assert(memcmp(buffer, buffer2, 2048), "Wrote too much");
  
  image_close(im);
  unlink("tests/testimg2.img");

  return NULL;
}

char *test_image_readwrite2()
{
  char buffer[2048];
  char buffer2[2048];
  memset(buffer2, 0, 2048);
  FILE *fp = fopen("/dev/urandom",  "r");
  fread(buffer, 2048, 1, fp);
  fclose(fp);

  image_t *im = image_load("tests/testimg3.img");
  mu_assert(im, "No image file");
  image_writeblocks(im, buffer, 5, 2);

  image_readblocks(im, buffer2, 5, 1);

  mu_assert(!memcmp(buffer, buffer2, 512), "Read did not return same as write");
  mu_assert(memcmp(buffer, buffer2, 1024), "Read too much");

  image_readblocks(im, buffer2, 5, 4);
  mu_assert(memcmp(buffer, buffer2, 2048), "Wrote too much");
  
  image_close(im);
  unlink("tests/testimg2.img");

  return NULL;
}

char *all_tests() {
  mu_suite_start();
  mu_run_test(test_image_load);
  mu_run_test(test_image_new);
  mu_run_test(test_image_readwrite);
  mu_run_test(test_image_readwrite2);
  return NULL;
}

RUN_TESTS(all_tests);
