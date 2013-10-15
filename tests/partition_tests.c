#include "minunit.h"
#include <partition.h>
#include <errno.h>

char *test_load_mbr()
{
  image_t *im = load_image("tests/testimg.img");
  MBR_entry_t *MBR = load_mbr(im);

  mu_assert(MBR != NULL, "Did not return MBR");

  mu_assert(MBR[0].boot_indicator == 0x80, "First partition not bootable in test image");
  mu_assert(MBR[0].system_id == 0x83, "Wrong system indicator in test image");

  free(MBR);
  free_image(im);
  return NULL;
}

char *test_partition()
{
  image_t *im = new_image("tests/testimg2.img", 21030912);

  size_t sizes[] = {10000000, 5000000, 0, 5000000};
  partition(im, sizes, 0);

  MBR_entry_t *MBR = load_mbr(im);
  mu_assert(MBR[0].boot_indicator == 0x80, "Unbootable first partition");
  mu_assert(MBR[0].system_id == 0x83, "Wrong system id on partition 0");
  mu_assert(MBR[1].system_id == 0x83, "Wrong system id on partition 1");
  mu_assert(MBR[2].system_id == 0x0, "Wrong system id on partition 2");
  mu_assert(MBR[3].system_id == 0x83, "Wrong system id on partition 3");

  mu_assert(MBR[0].num_sectors >= 10000000/BLOCK_SIZE, "Wrong size on partition 0");
  mu_assert(MBR[0].num_sectors-1 <= 10000000/BLOCK_SIZE, "Wrong size on partition 0");
  mu_assert(MBR[1].num_sectors >= 5000000/BLOCK_SIZE, "Wrong size on partition 1");
  mu_assert(MBR[1].num_sectors-1 <= 5000000/BLOCK_SIZE, "Wrong size on partition 1");
  mu_assert(MBR[2].num_sectors == 0, "Wrong size on partition 2");
  mu_assert(MBR[3].num_sectors >= 5000000/BLOCK_SIZE, "Wrong size on partition 3");
  mu_assert(MBR[3].num_sectors-1 <= 5000000/BLOCK_SIZE, "Wrong size on partition 3");

  free(MBR);
  free_image(im);

  return NULL;
}

char *test_get_partition()
{

  image_t *im = new_image("tests/testimg2.img", 21030912);

  size_t sizes[] = {10000000, 5000000, 0, 5000000};
  partition(im, sizes, 0);

  partition_t *p = get_partition(im, 1);

  mu_assert(p, "No partition returned");

  mu_assert(p->im == im, "Wrong image file in partition");
  mu_assert(p->partition == 1, "Wrong partition chosen");
  mu_assert(p->offset >= 10000000, "Wrong partition offset");
  mu_assert(p->offset <= 10000000+BLOCK_SIZE, "Wrong partition offset");
  mu_assert(p->length >= 5000000, "Wrong partition size");
  mu_assert(p->length <= 5000000+BLOCK_SIZE, "Wrong partition size");

  free_partition(p);
  free_image(im);

  return NULL;
}

char *all_tests() {
  mu_suite_start();
  mu_run_test(test_load_mbr);
  mu_run_test(test_partition);
  mu_run_test(test_get_partition);
  return NULL;
}

RUN_TESTS(all_tests);
