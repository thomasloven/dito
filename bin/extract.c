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
  for(i = 0; s[i]; s[i]==':'?i++:s++);
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
  printf("usage: %s image:partition output\n", argv[0]);
}

int main(int argc, const char *argv[])
{

  int retval = 0;
  path_t *path = 0;
  image_t *im = 0;
  FILE *output = 0;
  void *buffer = 0;

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

  MBR_entry_t *mbr = &im->mbr[path->partition];

  output = fopen(argv[2], "w+");

  unsigned int i;
  buffer = malloc(512);
  fseek(im->file, 512*mbr->start_LBA, SEEK_SET);
  for(i = 0; i < mbr->num_sectors; i++)
  {
    int readcount = fread(buffer, 1, 512, im->file);
    fwrite(buffer, 1, readcount, output);
  }
  printf("Extracted %d disk sectors (%d bytes) of partition %d.\n", i, i*512, path->partition+1);

  return 0;

end:
  if(path)
    free_path(path);
  if(im)
    image_close(im);
  if(output)
    fclose(output);
  if(buffer)
    free(buffer);
  return retval;
}
