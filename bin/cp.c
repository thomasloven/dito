#include <stdio.h>
#include <dito.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

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

size_t iread(void *ptr, size_t size, size_t nitems, file_t *file)
{
  int ret = 0;
  if(file->type == ftype_image)
  {
    ret = fs_read(file->fs, file->ino, ptr, size*nitems, file->offset);
    file->offset += ret;
  }
  if(file->type == ftype_native)
  {
    ret = fread(ptr, size, nitems, file->file);
  }

  return ret;
}

size_t iwrite(void *ptr, size_t size, size_t nitems, file_t *file)
{
  int ret = 0;
  if(file->type == ftype_image)
  {
    ret = fs_write(file->fs, file->ino, ptr, size*nitems, file->offset);
    file->offset += ret;
  }
  if(file->type == ftype_native)
  {
    ret = fwrite(ptr, size, nitems, file->file);
  }
  if(file->type == ftype_std)
  {
    ret = fwrite(ptr, size, nitems, stdout);
  }
  
  return ret;
}

void usage(const char *argv[])
{
  printf("usage: %s [type:image_file:partition:]source [type:image_file:partition:]dest\n", argv[0]);
  printf("       %s - [type:image_file:partition:]dest\n", argv[0]);
  printf("       %s [type:image_file:partition:]source -\n", argv[0]);
}

int main(int argc, const char *argv[])
{
  int retval = 0;
  path_t *src_path = 0;
  path_t *dst_path = 0;
  file_t src_f = {0, 0, 0, 0, 0, 0};
  file_t dst_f = {0, 0, 0, 0, 0, 0};
  image_t *src_im = 0;
  image_t *dst_im = 0;
  partition_t *src_p = 0;
  partition_t *dst_p = 0;
  fs_t *src_fs = 0;
  fs_t *dst_fs = 0;
  void *buffer = 0;
  FILE *tmpf = 0;
  fstat_t *st = 0;

  if(argc != 3)
  {
    usage(argv);
    retval = 1;
    goto end;
  }

  if(!(src_path = parse_path(argv[1])))
  {
    fprintf(stderr, "%s: %s: Invalid path\n", argv[0], argv[1]);
    retval = 1;
    goto end;
  }

  if(!(dst_path = parse_path(argv[2])))
  {
    fprintf(stderr, "%s: %s: Invalid path\n", argv[0], argv[2]);
    retval = 1;
    goto end;
  }

  // Load source image
  if(src_path->type != native && src_path->type != std)
  {
    if(!(src_im = image_load(src_path->image)))
    {
      fprintf(stderr, "%s: %s: Could not open source image file\n", argv[0], argv[1]);
      retval = 1;
      goto end;
    }
    if(!(src_p = partition_open(src_im, src_path->partition)))
    {
      fprintf(stderr, "%s: %s: Could not open source partition\n", argv[0], argv[1]);
      retval = 1;
      goto end;
    }
    if(!(src_fs = fs_load(src_p, src_path->type)))
    {
      fprintf(stderr, "%s: %s: Could not open source file system\n", argv[0], argv[1]);
      retval = 1;
      goto end;
    }
  }

  // Load destination image
  if(dst_path->type != native && dst_path->type != std)
  {
    if(src_path->image && !strcmp(dst_path->image,src_path->image))
    {
      dst_im = src_im;
      if(dst_path->partition == src_path->partition)
      {
        dst_p = src_p;
        dst_fs = src_fs;
      } else {
        if(!(dst_p = partition_open(dst_im, dst_path->partition)))
        {
          fprintf(stderr, "%s: %s: Could not open destination partition\n", argv[0], argv[2]);
          retval = 1;
          goto end;
        }
        if(!(dst_fs = fs_load(dst_p, dst_path->type)))
        {
          fprintf(stderr, "%s: %s: Could not open destination file system\n", argv[0], argv[2]);
          retval = 1;
          goto end;
        }
      }
    } else {
      if(!(dst_im = image_load(dst_path->image)))
      {
        fprintf(stderr, "%s: %s: Could not open destination image file\n", argv[0], argv[2]);
        retval = 1;
        goto end;
      }
      if(!(dst_p = partition_open(dst_im, dst_path->partition)))
      {
        fprintf(stderr, "%s: %s: Could not open destination partition\n", argv[0], argv[2]);
        retval = 1;
        goto end;
      }
      if(!(dst_fs = fs_load(dst_p, dst_path->type)))
      {
        fprintf(stderr, "%s: %s: Could not open destination file system\n", argv[0], argv[2]);
        retval = 1;
        goto end;
      }
    }
  }

  buffer = malloc(BUFFER_SIZE);
  int readcount = 0;
  if(src_path->type == std)
  {
    // If source is stdin
    // read data to temporary file
    char tmpfilename[256];
    sprintf(tmpfilename, "/tmp/imtools_%d.tmp", getpid());
    tmpf = fopen(tmpfilename, "w+");
    while((readcount = fread(buffer, 1, BUFFER_SIZE, stdin)))
    {
      fwrite(buffer, 1, readcount, tmpf);
    }
    if(ferror(stdin))
    {
      fprintf(stderr, "%s: Error reading from stdin\n", argv[0]);
      retval = 0;
      goto end;
    }
    fclose(tmpf);
    src_path->type = native;
    src_path->path = strdup(tmpfilename);
  } 
  if(src_path->type == native)
  {
    src_f.type = ftype_native;
    src_f.file = fopen(src_path->path, "r");
  } else {
    src_f.type = ftype_image;
    src_f.fs = src_fs;
    src_f.ino = fs_find(src_fs, src_path->path);
    src_f.offset = 0;
  }

  if(dst_path->type == std)
  {
    dst_f.type = ftype_std;
  } else if(dst_path->type == native) {
    dst_f.type = ftype_native;
    dst_f.file = fopen(dst_path->path, "w+");
  } else {
    dst_f.type = ftype_image;
    dst_f.fs = dst_fs;
    if(src_f.type == ftype_image)
    {
      st = fs_fstat(src_f.fs, src_f.ino);
    } else {
      // Create new file in the image
      st = malloc(sizeof(fstat_t));
      struct stat src_st;
      stat(src_path->path, &src_st);
      st->size = src_st.st_size;
      st->mode = S_CHR | S_RUSR | S_XGRP | S_WOTH;
      if((src_st.st_mode & S_IFIFO) == S_IFIFO) st->mode = S_FIFO;
      if((src_st.st_mode & S_IFCHR) == S_IFCHR) st->mode = S_CHR;
      if((src_st.st_mode & S_IFDIR) == S_IFDIR) st->mode = S_DIR;
      if((src_st.st_mode & S_IFBLK) == S_IFBLK) st->mode = S_BLK;
      if((src_st.st_mode & S_IFREG) == S_IFREG) st->mode = S_REG;
      if((src_st.st_mode & S_IFLNK) == S_IFLNK) st->mode = S_LINK;
      if((src_st.st_mode & S_IFSOCK) == S_IFSOCK) st->mode = S_SOCK;
      st->mode |= src_st.st_mode & 0777;
    #ifdef __APPLE__
      st->atime = src_st.st_atimespec.tv_sec;
      st->ctime = src_st.st_ctimespec.tv_sec;
      st->mtime = src_st.st_mtimespec.tv_sec;
    #else
      st->atime = src_st.st_atime;
      st->ctime = src_st.st_ctime;
      st->mtime = src_st.st_mtime;
    #endif
    }
    dst_f.ino = fs_touchp(dst_fs, st, dst_path->path);
  }



  if(src_path->type == native)
    printf(" Source: %s\n", src_path->path);
  else if(src_path->type == std)
    printf(" Source: stdin\n");
  else
    printf(" Source: %d %s %d %s\n", src_path->type, src_path->image, src_path->partition, src_path->path);


  while((readcount = iread(buffer, 1, BUFFER_SIZE, &src_f)))
  {
    iwrite(buffer, 1, readcount, &dst_f);
  }


end:
  if(src_f.type == ftype_native && src_f.file)
    fclose(src_f.file);
  if(dst_f.type == ftype_native && dst_f.file)
    fclose(dst_f.file);
  if(src_fs)
    fs_close(src_fs);
  if(dst_fs && dst_fs != src_fs)
    fs_close(dst_fs);

  if(src_p)
    partition_close(src_p);
  if(dst_p && dst_p != src_p)
    partition_close(dst_p);

  if(src_im)
    image_close(src_im);
  if(dst_im && dst_im != src_im)
    image_close(dst_im);

  if(src_path)
    free_path(src_path);
  if(dst_path)
    free_path(dst_path);
  if(buffer)
    free(buffer);
  if(st)
    free(st);

  return retval;
}
