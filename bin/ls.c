#include <stdio.h>
#include <image.h>
#include <partition.h>
#include <fs.h>
#include <string.h>
#include <stdlib.h>

void usage(const char *argv[])
{
  printf("usage: %s image_file:partition path\n", argv[0]);
}

int main(int argc, const char *argv[])
{
  int retval = 0;
  char *image_name = 0;
  int partition = -1;
  image_t *im = 0;
  partition_t *p = 0;
  fs_t *fs = 0;
  dirent_t *de = 0;
  if(argc < 2 || argc > 3)
  {
    usage(argv);
    retval = 1;
    goto end;
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
      retval = 1;
      goto end;
  }
    
  im = image_load(image_name);
  if(!im)
  {
    retval = 1;
    goto end;
  }
  p = partition_open(im, partition);
  if(!p)
  {
    retval = 0;
    goto end;
  }
  fs = fs_load(p, ext2);

  INODE dir = fs_find(fs, argv[2]);
  if(!dir)
  {
    retval = 1;
    goto end;
  }
  int i = 0;
  while((de = fs_readdir(fs, dir, i)))
  {
    printf("%s %d\n", de->name, de->ino);
    i++;
  }

  retval = 0;

end:
  if(de)
  {
    free(de);
  }
  if(fs)
  {
    fs_close(fs);
  }
  if(p)
  {
    partition_close(p);
  }
  if(im)
  {
    image_close(im);
  }
  if(image_name)
    free(image_name);
  return retval;
}
