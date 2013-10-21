#include "minunit.h"
#include <errno.h>
#include <filesystem.h>



char *test_unknown_filesystem()
{
  filesystem_t *fs = calloc(1, sizeof(filesystem_t));
  mu_assert(check_fs(fs) == -1, "Check FS returned result other than -1");
  mu_assert(read_directory(fs, "/", 1) == 0, "Read directory returned result other than -1");
  mu_assert(mkdir(fs, "/test") == -1, "Mkdir returned result other than -1");
  mu_assert(rmdir(fs, "/test") == -1, "Rmdir returned result other than -1");
  mu_assert(read_file(fs, "/test", 0, 0, 0) == (size_t)-1, "Read file returned result other than -1");
  mu_assert(write_file(fs, "/test", 0, 0, 0) == (size_t)-1, "Write file returned result other than -1");
  mu_assert(rm_file(fs, "/test") == -1, "Rm file returned result other than -1");
  mu_assert(ln_file(fs, "/test", "/test2") == -1, "Ln file returned result other than -1");
  free(fs);
  return NULL;
}

char *all_tests() {
  mu_suite_start();
  mu_run_test(test_unknown_filesystem);
  return NULL;
}

RUN_TESTS(all_tests);
