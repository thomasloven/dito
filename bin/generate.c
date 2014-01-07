#include <stdio.h>
#include <dito.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 512

void usage(const char *argv[])
{
  printf("usage: %s image_file size1 [size2] [size3] [size4]\n", argv[0]);
}

size_t parse_size(const char *size)
{
  size_t bytes = atoi(size);
  switch(size[strlen(size)-1])
  {
    case 'G':
      bytes = bytes * 1024;
    case 'M':
      bytes = bytes * 1024;
    case 'K':
      bytes = bytes * 1024;
    default:
      ;
  }

  bytes = bytes + (BLOCK_SIZE - (bytes % BLOCK_SIZE));
  return bytes;
}

int main(int argc, const char *argv[])
{
  int retval = 1;
  size_t sizes[] = {0, 0, 0, 0};
  image_t *im = 0;
  char *filename = 0;

  if(argc < 3 || argc >6)
  {
    usage(argv);
    retval = 1;
    goto end;
  }

  sizes[0] = parse_size(argv[2]);
  if(argc > 3)
    sizes[1] = parse_size(argv[3]);
  if(argc > 4)
    sizes[2] = parse_size(argv[4]);
  if(argc > 5)
    sizes[3] = parse_size(argv[5]);

  filename = strdup(argv[1]);
  if(!(im = image_new(filename, sizes, 0)))
  {
    fprintf(stderr, "%s: Failed to create disk image\n", argv[0]);
    retval = 1;
    goto end;
  }

  printf(" Image file created with CHS: %d %d %d\n", im->cylinders, im->heads, im->sectors);

end:
  if(im)
    image_close(im);
  if(filename)
    free(filename);
  return retval;
}
