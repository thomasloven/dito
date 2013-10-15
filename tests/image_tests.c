#include "minunit.h"

char *test_load_image()
{
  return NULL;
}

char *test_load_mbr()
{
  return NULL;
}

char *all_tests() {
  mu_suite_start();
  mu_run_test(test_load_image);
  mu_run_test(test_load_mbr);
  return NULL;
}

RUN_TESTS(all_tests);
