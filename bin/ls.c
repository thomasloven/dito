#include <stdio.h>
#include <image.h>
#include <partition.h>
#include <fs.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

void usage(const char *argv[])
{
  printf("usage: %s image_file:partition [-l] [path]\n", argv[0]);
}

int main(int argc, const char *argv[])
{
  int retval = 0;
  char *image_name = 0;
  char *path = 0;
  int partition = -1;
  int detailed = 0;
  image_t *im = 0;
  partition_t *p = 0;
  fs_t *fs = 0;
  dirent_t *de = 0;
  fstat_t *st = 0;
  if(argc < 2 || argc > 4)
  {
    usage(argv);
    retval = 1;
    goto end;
  }
  if(argc == 2)
  {
    path = strdup("/");
  }
  if(argc == 3)
  {
    if(strcmp(argv[2], "-l"))
    {
      path = strdup(argv[2]);
    } else {
      detailed = 1;
      path = strdup("/");
    }
  }
  if(argc == 4)
  {
    if(strcmp(argv[2], "-l"))
    {
      usage(argv);
      goto end;
    } else {
      detailed = 1;
      path = strdup(argv[3]);
    }
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
    fprintf(stderr, "%s: Image file could not load\n", argv[0]);
    retval = 1;
    goto end;
  }
  p = partition_open(im, partition);
  if(!p)
  {
    fprintf(stderr, "%s: Partition could not be read\n", argv[0]);
    retval = 0;
    goto end;
  }
  fs = fs_load(p, ext2);

  INODE dir = fs_find(fs, path);
  if(!dir)
  {
    fprintf(stderr, "%s: %s: No such file or directory\n", argv[0], path);
    retval = 1;
    goto end;
  }
  st = fs_fstat(fs, dir);
  if((st->mode & S_DIR) != S_DIR)
  {
    fprintf(stderr, "%s: %s: Not a directory\n", argv[0], path);
    retval = 1;
    goto end;
  }
  int i = 0;
  while((de = fs_readdir(fs, dir, i)))
  {
    if(detailed)
    {
      st = fs_fstat(fs, de->ino);
      printf("%c%c%c%c%c%c%c%c%c%c", \
          ((st->mode & S_DIR) == S_DIR)?'d':'-', \
          ((st->mode & S_RUSR) == S_RUSR)?'r':'-', \
          ((st->mode & S_WUSR) == S_WUSR)?'w':'-', \
          ((st->mode & S_XUSR) == S_XUSR)?'x':'-', \
          ((st->mode & S_RGRP) == S_RGRP)?'r':'-', \
          ((st->mode & S_WGRP) == S_WGRP)?'w':'-', \
          ((st->mode & S_XGRP) == S_XGRP)?'x':'-', \
          ((st->mode & S_ROTH) == S_ROTH)?'r':'-', \
          ((st->mode & S_WOTH) == S_WOTH)?'w':'-', \
          ((st->mode & S_XOTH) == S_XOTH)?'x':'-');
      time_t mtime = st->mtime;
      char buffer[25];
      strftime(buffer, 25, "%d %b %H:%M", gmtime(&mtime));
      printf("\t %ld \t %s \t %s\n", st->size, buffer, de->name);
    } else {
      printf("%s \t", de->name);
    }
    i++;
  }
  printf("\n");

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
  if(path)
    free(path);
  return retval;
}
