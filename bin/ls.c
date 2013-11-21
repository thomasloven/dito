#include <stdio.h>
#include <dito.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

typedef struct path_st
{
  fs_type_t type;
  char *image;
  int partition;
  char *path;
} path_t;

path_t *parse_path(const char *input)
{
  path_t *path = malloc(sizeof(path_t));
  path->type = 0;
  path->image = 0;
  path->partition = -1;
  path->path = 0;
  int i;
  char *str = strdup(input);
  char *s = str;

  // Count number of :
  for(i = 0; s[i]; s[i]==':'?i++:(int)(s++));
  if(i == 0)
  {
    // File is not in an image
    if(!strcmp(str, "-"))
    {
      // File is stdin or stdout
      path->type = std;
    } else {
      // File is on disk
      path->type = native;
      path->path = strdup(str);
    }
    free(str);
    return path;
  }
  if(i != 3)
  {
    // Path is invalid
    free(str);
    return 0;
  }

  // File is in an image
  i = 0;

  // Parse filesystem type
  s = &str[i];
  while(str[i] != ':') i++;
  str[i] = '\0';
  if(!strcmp(s, "ext2")) path->type = ext2;
  else if(!strcmp(s, "fat")) path->type = fat;
  else if(!strcmp(s, "sfs")) path->type = sfs;
  else if(!strcmp(s, "ntfs")) path->type = ntfs;
  else if(!strcmp(s, "hfs")) path->type = hfs;
  else path->type = unknown;

  // Parse image filename
  i++;
  s = &str[i];
  while(str[i] != ':') i++;
  str[i] = '\0';
  path->image = strdup(s);

  // Parse partition
  i++;
  s = &str[i];
  if(s[0]>= '1' && s[0]<= '4')
  {
    path->partition = (int)(s[0]-'1');
  } else {
    // Bad partition number
    free(str);
    return 0;
  }
  
  //Parse path within image
  i+=2;
  s = &str[i];
  path->path = strdup(s);

  free(str);
  return path;

}

void free_path(path_t *p)
{
  if(p->image)
    free(p->image);
  if(p->path)
    free(p->path);
  free(p);
}

void usage(const char *argv[])
{
  printf("usage: %s [-l] type:image_file:partition:path\n", argv[0]);
}

int main(int argc, const char *argv[])
{
  int retval = 0;
  int detailed = 0;
  char *pth = 0;
  path_t *path = 0;
  image_t *im = 0;
  partition_t *p = 0;
  fs_t *fs = 0;
  dirent_t *de = 0;
  fstat_t *st = 0;

  if(argc < 2 || argc > 3)
  {
    usage(argv);
      printf("argc= fel\n");
    retval = 1;
    goto end;
  }
  if(argc == 3)
  {
    if(!strcmp(argv[1], "-l"))
    {
      detailed = 1;
      pth = strdup(argv[2]);
    } else {
      usage(argv);
      printf("argc=3\n");
      retval = 1;
      goto end;
    }
  } else {
    pth = strdup(argv[1]);
  }
  if(!(path = parse_path(pth)))
  {
      printf("parse\n");
    usage(argv);
    retval = 1;
    goto end;
  }

  if(!(im = image_load(path->image)))
  {
    fprintf(stderr, "%s: %s: Could not open image file\n", argv[0], pth);
    retval = 1;
    goto end;
  }
  if(!(p = partition_open(im, path->partition)))
  {
    fprintf(stderr, "%s: %s: Could not open partition\n", argv[0], pth);
    retval = 1;
    goto end;
  }
  if(!(fs = fs_load(p, path->type)))
  {
    fprintf(stderr, "%s: %s: Could not load filesystem\n", argv[0], pth);
    retval = 1;
    goto end;
  }

    

  INODE dir = fs_find(fs, path->path);
  if(!dir)
  {
    fprintf(stderr, "%s: %s: No such file or directory\n", argv[0], path->path);
    retval = 1;
    goto end;
  }
  st = fs_fstat(fs, dir);
  if((st->mode & S_DIR) != S_DIR)
  {
    fprintf(stderr, "%s: %s: Not a directory\n", argv[0], path->path);
    retval = 1;
    goto end;
  }
  free(st);
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
      free(st);
    } else {
      printf("%s \t", de->name);
    }
    i++;
    free(de->name);
    free(de);
  }
  if(!detailed) printf("\n");

  retval = 0;

end:
  if(st)
    free(st);
  if(fs)
    fs_close(fs);
  if(p)
    partition_close(p);
  if(im)
    image_close(im);
  if(path)
    free_path(path);
  if(pth)
    free(pth);
  return retval;
}
