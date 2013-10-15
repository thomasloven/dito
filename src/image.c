#include "image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

image_t *load_image(char *filename)
{
  image_t *image = malloc(sizeof(image_t));
  image->filename = strdup(filename);
  image->file = fopen(filename, "w+");

  return image;
}

MBR_entry_t *load_mbr(image_t *image)
{
  MBR_entry_t *MBR = calloc(4, sizeof(MBR_entry_t));
  fseek(image->file, MBR_OFFSET, SEEK_SET);
  fread(MBR, sizeof(MBR_entry_t), 4, image->file);
  return MBR;
}
