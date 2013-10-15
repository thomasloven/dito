#include "minunit.h"
#include <image.h>
#include <errno.h>

char *test_load_image()
{
  image_t *im = load_image("tests/testimg.img");

  mu_assert(im != NULL, "Did not return image");
  mu_assert(im->file != NULL, "Did not open a file");

  mu_assert(im->cylinders == 0x147, "Test image has wrong number of cylinders");
  mu_assert(im->heads == 0x2, "Test image has wrong number of heads");
  mu_assert(im->sectors == 0x3f, "Test image has wrong number of sectors");

  free_image(im);

  return NULL;
}

char *test_new_image()
{
  image_t *im = new_image("tests/testimg2.img", 21030912);

  mu_assert(im != NULL, "Did not return image");
  mu_assert(im->file != NULL, "Did not open a file");

  mu_assert(im->cylinders == 0x147, "New image has wrong number of cylinders");
  mu_assert(im->heads == 0x2, "New image has wrong number of heads");
  mu_assert(im->sectors == 0x3f, "New image has wrong number of sectors");

  free_image(im);

  return NULL;
}


char *all_tests() {
  mu_suite_start();
  mu_run_test(test_load_image);
  mu_run_test(test_new_image);
  return NULL;
}

RUN_TESTS(all_tests);
