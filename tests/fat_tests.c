#include "minunit.h"
#include <dito.h>
#include <errno.h>
#include "../src/fat.h"

char *test_fat_load()
{
  image_t *im = image_load("tests/fat.img");
  mu_assert(im, "No image");
  partition_t *p = partition_open(im, 0);
  mu_assert(p, "No partition");

  fs_t *fs = fs_load(p, fat);
  mu_assert(fs, "No FAT12");
  mu_assert(fat_bits(fs) == 12, "Wrong FAT type (Not 12)");
  fat_bpb_t *bpb = ((fat_data_t *)fs->data)->bpb;
  printf("Root cluster: %d\n", ((fat_data_t *)fs->data)->inodes->cluster);
  printf(" Bytes per sector: %d\n", bpb->bytes_per_sector);
  printf(" Bytes per cluster: %d (%d sectors)\n", bpb->bytes_per_sector*bpb->sectors_per_cluster, bpb->sectors_per_cluster);
  printf(" Reserved sectors: %d\n", bpb->reserved_sectors);
  printf(" First fat at: %d\n", fat_fat_start(fs));
  printf(" Fat tables: %d\n", bpb->fat_count);
  printf(" Fat size: %d (%d sectors)\n", bpb->sectors_per_fat*bpb->bytes_per_sector, bpb->sectors_per_fat);
  printf("  Fat table value %x\n", fat_read_fat(fs, 0));
  printf("  Fat table value %x\n", fat_read_fat(fs, 1));
  printf("  Fat table value %x\n", fat_read_fat(fs, 2));
  printf("  Fat table value %x\n", fat_read_fat(fs, 3));
  printf("  Fat table value %x\n", fat_read_fat(fs, 0x7f-2));
  fs_close(fs);
  partition_close(p);

  p = partition_open(im, 1);
  mu_assert(p, "No partition");
  fs = fs_load(p, fat);
  mu_assert(fs, "No FAT16");
  mu_assert(fat_bits(fs) == 16, "Wrong FAT type (Not 16)");
  printf("Root cluster: %d\n", ((fat_data_t *)fs->data)->inodes->cluster);
  fs_close(fs);
  partition_close(p);

  p = partition_open(im, 2);
  mu_assert(p, "No partition");
  fs = fs_load(p, fat);
  mu_assert(fs, "No FAT32");
  mu_assert(fat_bits(fs) == 32, "Wrong FAT type (Not 32)");
  printf("Root cluster: %d\n", ((fat_data_t *)fs->data)->inodes->cluster);
  fs_close(fs);
  partition_close(p);
  image_close(im);

  return NULL;
}

char *test_fat_readcluster()
{
  image_t *im = image_load("tests/fat.img");
  mu_assert(im, "No image");
  partition_t *p = partition_open(im, 0);
  mu_assert(p, "No partition");

  fs_t *fs = fs_load(p, fat);
  mu_assert(fs, "No FAT12");
  mu_assert(fat_bits(fs) == 12, "Wrong FAT type (Not 12)");

  /* fat_bpb_t *bpb = ((fat_data_t *)fs->data)->bpb; */

  /* return (bpb->reserved_sectors + bpb->fat_count*bpb->sectors_per_fat)/bpb->sectors_per_cluster; */

  fs_close(fs);
  partition_close(p);
  image_close(im);

  return NULL;
}

char *test_fat_write_fat()
{
  image_t *im = image_load("tests/fat.img");
  mu_assert(im, "No image");
  partition_t *p = partition_open(im, 0);
  mu_assert(p, "No partition");

  fs_t *fs = fs_load(p, fat);
  mu_assert(fs, "No FAT12");
  mu_assert(fat_bits(fs) == 12, "Wrong FAT type (Not 12)");

  fat_write_fat(fs, 14, 0x12);
  fat_write_fat(fs, 15, 0xAB);
  fat_write_fat(fs, 16, 0xCD);

  mu_assert(fat_read_fat(fs, 14) == 0x12, "Offset 14 error");
  mu_assert(fat_read_fat(fs, 15) == 0xAB, "Offset 15 error");
  mu_assert(fat_read_fat(fs, 16) == 0xCD, "Offset 16 error");

  fs_close(fs);
  partition_close(p);
  image_close(im);

  return NULL;
}

char *test_fat_make_shortname()
{
  printf("longfilename.txt : %s\n", fat_make_shortname("longfilename.txt"));
  printf("longfilename.c : %s\n", fat_make_shortname("longfilename.c"));
  printf("name.txt : %s\n", fat_make_shortname("name.txt"));
  printf("name.c : %s\n", fat_make_shortname("name.c"));
  return NULL;
}

char *test_fat_longname()
{
  void *de = calloc(10,32);

  fat_write_longname(de, "File with very long filename.ext");
  printf(" Filename read: %s\n", fat_read_longname(de));
  free(de);
  return NULL;
}

char *all_tests() {
  mu_suite_start();
  mu_run_test(test_fat_load);
  mu_run_test(test_fat_readcluster);
  mu_run_test(test_fat_write_fat);
  mu_run_test(test_fat_make_shortname);
  mu_run_test(test_fat_longname);
  return NULL;
}

RUN_TESTS(all_tests);
