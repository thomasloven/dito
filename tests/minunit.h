#undef NDEBUG
#ifndef _minunit_h
#define _minunit_h

#include <stdio.h>
#include <stdlib.h>

#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define clean_errno() (errno = 0 ? "None" : strerror(errno))

#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define log_warn(M, ...) fprintf(stderr, "[WARN] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)
#define log_info(M, ...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define mu_suite_start() char *message = NULL

#define mu_assert(test, message) if(!(test)) { log_err(message); return message; }
#define mu_run_test(test) debug("\n-----%s", "" #test); \
  message = test(); tests_run++; if(message) return message;

#define RUN_TESTS(name) int main(int argc, char *argv[]) {\
  argc = 1; \
  debug("----- RUNNING: %s", argv[0]);\
  printf("----\nRUNNING: %s\n", argv[0]);\
  char *result = name();\
  if (result != 0) {\
    printf("\e[31mFAILED\e[0m: %s\n", result);\
  }\
  else {\
    printf("\e[32mALL TESTS PASSED\e[0m\n");\
  }\
  printf("Tests run: %d\n", tests_run);\
  exit(result != 0);\
}

int tests_run;

#endif
