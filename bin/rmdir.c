#include <stdio.h>
#include <dito.h>
#include <string.h>
#include <stdlib.h>


typedef struct path_st
{
  fs_type_t type;
  char *image;
  int partition;
  char *path;
} path_t;

enum ftype {
  ftype_std,
  ftype_native,
  ftype_image
};
typedef struct file_st
{
  enum ftype type;
  fs_t *fs;
  INODE ino;
  size_t offset;
  FILE *file;
  size_t size;
} file_t;

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
  else if(!strcmp(s, "fat16")) path->type = fat16;
  else if(!strcmp(s, "fat32")) path->type = fat32;
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
  printf("usage: %s type:image_file:partition:path\n", argv[0]);
}

int main(int argc, const char *argv[])
{
  int retval = 0;
  path_t *path = 0;
  image_t *im = 0;
  partition_t *p = 0;
  fs_t *fs = 0;
  char *pth = 0;
  dirent_t *de = 0;
  fstat_t *st = 0;

  if(argc != 2)
  {
    usage(argv);
    retval = 1;
    goto end;
  }

  if(!(path = parse_path(argv[1])))
  {
    fprintf(stderr, "%s: %s: Invalid path\n", argv[0], argv[1]);
    retval = 1;
    goto end;
  }

  if(!(im = image_load(path->image)))
  {
    fprintf(stderr, "%s: %s: Could not open image file\n", argv[0], argv[1]);
    retval = 1;
    goto end;
  }
  if(!(p = partition_open(im, path->partition)))
  {
    fprintf(stderr, "%s: %s: Could not open partition\n", argv[0], argv[1]);
    retval = 1;
    goto end;
  }
  if(!(fs = fs_load(p, path->type)))
  {
    fprintf(stderr, "%s: %s: Could not open file system\n", argv[0], argv[1]);
    retval = 1;
    goto end;
  }

  INODE target = fs_find(fs, path->path);
  if(!(st = fs_fstat(fs, target)))
  {
    fprintf(stderr, "%s: %s: Fstat failed\n", argv[0], path->path);
    retval = 1;
    goto end;
  }

  if((st->mode & S_DIR) != S_DIR)
  {
    fprintf(stderr, "%s: %s is not a directory\n", argv[0], path->path);
    retval = 1;
    goto end;
  }

  if(fs_readdir(fs, target, 2))
  {
    fprintf(stderr, "%s: %s is not empty\n", argv[0], path->path);
    retval = 1;
    goto end;
  }
  

  pth = strdup(path->path);
  char *c = strrchr(pth, '/');
  c[0] = '\0';
  INODE parent = fs_find(fs, pth);

  int num = 0;
  while(1)
  {
    de = fs_readdir(fs, parent, num);
    if(!de)
    {
      fprintf(stderr, "%s: Path not found\n", argv[0]);
      retval = 1;
      goto end;
    }
    if(!strcmp(&c[1], de->name))
      break;
    free(de->name);
    free(de);
    num++;
  }

  retval = fs_rmdir(fs, parent, num);



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
  if(de)
  {
    free(de->name);
    free(de);
  }
  return retval;
}
