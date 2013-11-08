#include <dito.h>
#include <stdio.h>
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
  if(i != 1)
  {
    // Path is invalid
    free(str);
    return 0;
  }

  // File is in an image
  i = 0;

  // Parse image filename
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
  printf("usage: %s image:partition format\n", argv[0]);
}

int main(int argc, const char *argv[])
{
  int retval = 0;
  path_t *path = 0;
  image_t *im = 0;
  partition_t *p = 0;
  fs_t *fs = 0;

  if(argc != 3)
  {
    usage(argv);
    retval = 1;
    goto end;
  }

  path = parse_path(argv[1]);
  if(!path)
  {
    fprintf(stderr, "%s: %s: Invalid path\n", argv[0], argv[1]);
    usage(argv);
    retval = 1;
    goto end;
  }
  if(!(im = image_load(path->image)))
  {
    fprintf(stderr, "%s: %s: Could not open source image file\n", argv[0], argv[1]);
    retval = 1;
    goto end;
  }
  if(!(p = partition_open(im, path->partition)))
  {
    fprintf(stderr, "%s: %s: Could not open source image file\n", argv[0], argv[1]);
    retval = 1;
    goto end;
  }

  if(!strcmp(argv[2], "ext2")) path->type = ext2;
  else if(!strcmp(argv[2], "fat16")) path->type = fat16;
  else if(!strcmp(argv[2], "fat32")) path->type = fat32;
  else if(!strcmp(argv[2], "sfs")) path->type = sfs;
  else if(!strcmp(argv[2], "ntfs")) path->type = ntfs;
  else if(!strcmp(argv[2], "hfs")) path->type = hfs;
  else path->type = unknown;

  if(!(fs = fs_create(p, path->type)))
  {
    fprintf(stderr, "%s: Could not create filesystem %s\n", argv[0], argv[2]);
    retval = 1;
    goto end;
  }

end:
  if(fs)
    fs_close(fs);
  if(p)
    partition_close(p);
  if(im)
    image_close(im);
  if(path)
    free_path(path);
  return retval;
}
