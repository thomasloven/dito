#include "minunit.h"
#include <fs.h>

char *test_fs_load()
{
  return NULL;
}

char *all_tests() {
  mu_suite_start();
  mu_run_test(test_fs_load);
  return NULL;
}

RUN_TESTS(all_tests);
