#include "minunit.h"
#include <image.h>
#include <errno.h>

char *test_load_image()
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

char *test_new_image()
{
  size_t sizes[] = {10000000, 5000000, 0, 5000000};
  image_t *im = image_new("tests/testimg2.img", sizes, 0);

  mu_assert(im != NULL, "Did not return image");
  mu_assert(im->file != NULL, "Did not open a file");

  mu_assert(image_getsize(im) > (sizes[0]+sizes[1]+sizes[2]+sizes[3]), "Image size is too small");
  mu_assert(image_getsize(im) < 2*(sizes[0]+sizes[1]+sizes[2]+sizes[3]), "Image size seems much too large");

  mu_assert(!image_check(im), "Image check failed");

  image_close(im);

  return NULL;
}


char *all_tests() {
  mu_suite_start();
  mu_run_test(test_load_image);
  mu_run_test(test_new_image);
  return NULL;
}

RUN_TESTS(all_tests);
