#include "image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

CHS_t max_CHS_from_size(size_t size)
{
  CHS_t ret;
  ret.C = 1023;
  ret.H = 254;
  ret.S = 63;

  size_t real_size = ret.C*ret.H*ret.S;
  while(real_size > (size / BLOCK_SIZE))
  {
    ret.H -= 2;
    real_size = ret.C*ret.H*ret.S;
  }
  ret.H += 2;
  real_size = ret.C*ret.H*ret.S;

  while(real_size > (size / BLOCK_SIZE))
  {
    ret.C--;
    real_size = ret.C*ret.H*ret.S;
  }
  ret.C++;
  real_size = ret.C*ret.H*ret.S;

  while(real_size > (size / BLOCK_SIZE))
  {
    ret.S--;
    real_size = ret.C*ret.H*ret.S;
  }
  ret.S++;
  real_size = ret.C*ret.H*ret.S;
  if(real_size < (size/BLOCK_SIZE + ret.S + 1))
    ret.C++;

  return ret;

}

image_t *new_image(char *filename, size_t size)
{
  if(!filename)
    return 0;

  image_t *image = malloc(sizeof(image_t));
  image->filename = strdup(filename);
  image->file = fopen(filename, "w+");

  CHS_t CHS = max_CHS_from_size(size);

  image->cylinders = CHS.C;
  image->heads = CHS.H;
  image->sectors = CHS.S;

  return image;
}
image_t *load_image(char *filename)
{
  if(!filename)
    return 0;

  image_t *image = malloc(sizeof(image_t));
  image->filename = strdup(filename);
  image->file = fopen(filename, "r+");

  fseek(image->file, 0, SEEK_END);
  size_t filesize = ftell(image->file);

  CHS_t CHS = max_CHS_from_size(filesize);

  image->cylinders = CHS.C;
  image->heads = CHS.H;
  image->sectors = CHS.S;

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

CHS_t CHS_from_LBA(image_t *image, int lba)
{
  CHS_t ret;
  ret.C = lba/(image->heads*image->sectors);
  ret.H = (lba % image->heads*image->sectors)/image->sectors;
  ret.S = lba % image->sectors + 1;
  return ret;
}

size_t LBA_from_CHS(image_t *image, CHS_t chs)
{
  size_t ret = ((chs.C*image->heads)+chs.H)*image->sectors + chs.S -1;
  return ret;
}
