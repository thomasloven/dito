#include "minunit.h"
#include <partition.h>
#include <errno.h>



char *all_tests() {
  mu_suite_start();
  return NULL;
}

RUN_TESTS(all_tests);
