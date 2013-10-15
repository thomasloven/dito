#include "image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

image_t *load_image(char *filename)
{
  if(!filename)
    return 0;

  image_t *image = malloc(sizeof(image_t));
  image->filename = strdup(filename);
  image->file = fopen(filename, "r+");

  return image;
}

void free_image(image_t *image)
{
  if(!image)
    return;

  if(image->file)
    fclose(image->file);

  if(image->filename)
    free(image->filename);

  free(image);
}

MBR_entry_t *load_mbr(image_t *image)
{
  MBR_entry_t *MBR = calloc(4, sizeof(MBR_entry_t));
  fseek(image->file, MBR_OFFSET, SEEK_SET);
  fread(MBR, sizeof(MBR_entry_t), 4, image->file);
  return MBR;
}

void write_mbr(image_t *image, MBR_entry_t *MBR)
{
  if(!MBR)
    return;

  fseek(image->file, MBR_OFFSET, SEEK_SET);
  fwrite(MBR, sizeof(MBR_entry_t), 4, image->file);
}
