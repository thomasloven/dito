#include <stdio.h>
#include <dito.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

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
  printf("usage: %s type:image_file:partition:path \n", argv[0]);
}

int main(int argc, const char *argv[])
{
  int retval = 0;
  path_t *path = 0;
  image_t *im = 0;
  partition_t *p = 0;
  fs_t *fs = 0;
  fstat_t *parent_stat = 0;

  if(argc != 2)
  {
    usage(argv);
    return 1;
  }

  if(!(path = parse_path(argv[1])))
  {
    usage(argv);
    return 1;
  }
  if(path->type == std || path->type == native)
  {
    usage(argv);
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
    fprintf(stderr, "%s: %s: Could not load filesystem\n", argv[0], argv[1]);
    retval = 1;
    goto end;
  }

  char *name = strrchr(path->path, '/');
  name[0] = '\0';
  INODE parent = fs_find(fs, path->path);
  parent_stat = fs_fstat(fs, parent);
  if((parent_stat->mode & S_DIR) != S_DIR)
  {
    fprintf(stderr, "%s: %s: Not a directory\n", argv[0], path->path);
    retval = 1;
    goto end;
  }

  if(fs_mkdir(fs, parent, &name[1]))
  {
    fprintf(stderr, "%s: %s: Creating directory failed\n", argv[0], &name[1]);
    retval = 1;
    goto end;
  }

  retval = 0;

end:
  if(parent_stat)
    free(parent_stat);
  if(path)
    free_path(path);
  if(fs)
    fs_close(fs);
  if(p)
    partition_close(p);
  if(im)
    image_close(im);
  return retval;
}
