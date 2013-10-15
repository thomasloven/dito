#include "minunit.h"
#include <image.h>
#include <errno.h>

char *test_load_image()
{
  image_t *im = load_image("testimg.img");

  mu_assert(im != NULL, "Did not return image");
  mu_assert(im->file != NULL, "Did not open a file");

  free_image(im);

  return NULL;
}

char *test_load_mbr()
{
  image_t *im = load_image("testimg.img");
  MBR_entry_t *MBR = load_mbr(im);

  mu_assert(MBR != NULL, "Did not return MBR");

  mu_assert(MBR[0].boot_indicator == 0x80, "First partition not bootable in test image");
  mu_assert(MBR[0].system_id == 0x83, "Wrong system indicator in test image");

  free(MBR);
  free_image(im);
  return NULL;
}

char *all_tests() {
  mu_suite_start();
  mu_run_test(test_load_image);
  mu_run_test(test_load_mbr);
  return NULL;
}

RUN_TESTS(all_tests);
