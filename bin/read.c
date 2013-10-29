#include <stdio.h>
#include <image.h>
#include <partition.h>
#include <fs.h>
#include <string.h>
#include <stdlib.h>

void usage(const char *argv[])
{
  printf("usage: %s image_file:partition output [path]\n", argv[0]);
  printf("       %s image_file:partition - [path]\n", argv[0]);
}

int main(int argc, const char *argv[])
{
  int retval = 0;
  char *image_name = 0;
  int partition = -1;
  char *path = 0;
  FILE *output = 0;
  image_t *im = 0;
  partition_t *p = 0;
  fs_t *fs = 0;

  if(argc < 3 || argc > 4)
  {
    usage(argv);
    retval = 1;
    goto end;
  }
  if(strcmp(argv[2], "-"))
  {
    output = fopen(argv[2], "w+");
  } else {
    output = stdout;
  }
  if(argc == 4)
  {
    path = strdup(argv[3]);
  } else {
    path = strdup("/");
  }

  image_name = strdup(argv[1]);
  image_name[strlen(image_name)-2] = '\0';
  switch(argv[1][strlen(argv[1])-1])
  {
    case '4':
      partition++;
    case '3':
      partition++;
    case '2':
      partition++;
    case '1':
      partition++;
      break;
    default:
      fprintf(stderr, "Bad partition number\n");
      retval = 1;
      goto end;
  }

  im = image_load(image_name);
  if(!im)
  {
    fprintf(stderr, "Image file could not load\n");
    retval = 1;
    goto end;
  }
  p = partition_open(im, partition);
  if(!p)
  {
    fprintf(stderr, "Partition could not be read\n");
    retval = 0;
    goto end;
  }
  fs = fs_load(p, ext2);

  INODE ino = fs_find(fs, path);
  fstat_t *st = fs_fstat(fs, ino);
  void *buffer = malloc(st->size);
  fs_read(fs, ino, buffer, st->size, 0);
  fwrite(buffer, st->size, 1, output);

end:
  if(fs)
    fs_close(fs);
  if(p)
    partition_close(p);
  if(im)
    image_close(im);
  if(path)
    free(path);
  if(image_name)
    free(image_name);
  return retval;
}
