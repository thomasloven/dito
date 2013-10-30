#include "image.h"
#include <dito.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

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

image_t *image_new(char *filename, size_t sizes[4], int boot)
{
  if(!filename)
    return 0;
  if(boot < 0 || boot > 3)
    return 0;

  image_t *image = malloc(sizeof(image_t));
  image->filename = strdup(filename);
  image->file = fopen(filename, "w+");

  size_t size = sizes[0]+sizes[1]+sizes[2]+sizes[3];

  CHS_t chs = max_CHS_from_size(size);

  image->cylinders = chs.C;
  image->heads = chs.H;
  image->sectors = chs.S;

  size_t i = 0, pos=image->sectors*BLOCK_SIZE;
  for(i = 0; i < 4; i++)
  {
    size_t round_size = (sizes[i]+(sizes[i]%BLOCK_SIZE!=0)*BLOCK_SIZE);
    round_size = round_size - round_size%BLOCK_SIZE;
    size_t start_LBA = pos/BLOCK_SIZE;
    size_t end_LBA = start_LBA + round_size/BLOCK_SIZE;
    CHS_t start = CHS_from_LBA(image, start_LBA);
    CHS_t end = CHS_from_LBA(image, end_LBA);

    image->mbr[i].boot_indicator = 0;
    image->mbr[i].start_head = start.H;
    image->mbr[i].start_sector = 
      ((start.S &0x3F) << 2) | ((start.C & 0x300) >> 8);
    image->mbr[i].start_cylinder = (start.C & 0xFF);
    image->mbr[i].system_id = (sizes[i])?0x83:0x0;
    image->mbr[i].end_head = end.H;
    image->mbr[i].end_sector = ((end.S & 0x3F) << 2) | ((end.C & 0x300) >> 8);
    image->mbr[i].end_cylinder = (end.C & 0xFF);
    image->mbr[i].start_LBA = start_LBA;
    image->mbr[i].num_sectors = round_size/BLOCK_SIZE;

    pos += round_size;
  }

  image->mbr[boot].boot_indicator = 0x80;
  image->mbr_dirty = 1;

  ftruncate(fileno(image->file), size);

  return image;

}

image_t *image_load(char *filename)
{
    if(!filename)
      return 0;

    image_t *image = malloc(sizeof(image_t));
    image->filename = strdup(filename);
    image->file = fopen(filename, "r+");
    if(!image->file)
    {
      free(image);
      return 0;
    }

    fseek(image->file, 0, SEEK_END);
    size_t filesize = ftell(image->file);

    CHS_t CHS = max_CHS_from_size(filesize);

    image->cylinders = CHS.C;
    image->heads = CHS.H;
    image->sectors = CHS.S;

    fseek(image->file, MBR_OFFSET, SEEK_SET);
    fread(&image->mbr, sizeof(MBR_entry_t), 4, image->file);
    image->mbr_dirty = 0;

    return image;
}

void image_close(image_t *im)
{
  if(!im)
    return;

  if(im->file)
  {
    if(im->mbr_dirty)
    {
      fseek(im->file, MBR_OFFSET, SEEK_SET);
      fwrite(&im->mbr, sizeof(MBR_entry_t), 4, im->file);
    }
    fclose(im->file);
  }

  if(im->filename)
    free(im->filename);

  free(im);
}


MBR_entry_t *image_getmbr(image_t *im, int num)
{
  if(num < 0 || num > 3)
    return 0;
  return &im->mbr[num];
}

void image_setmbr(image_t *im, MBR_entry_t *mbr, int num)
{
  if(num < 0 || num > 3)
    return;
  memcpy(&im->mbr[num], mbr, sizeof(MBR_entry_t));
}

size_t image_get_partition_start(image_t *im, int num)
{
  if(num < 0 || num > 3)
    return 0;
  return im->mbr[num].start_LBA;
}

size_t image_get_partition_length(image_t *im, int num)
{
  if(num < 0 || num > 3)
    return 0;
  return im->mbr[num].num_sectors;
}

size_t image_getsize(image_t *im)
{
  if(!im)
    return 0;

  size_t blocks = im->cylinders*im->heads*im->sectors;

  return blocks*BLOCK_SIZE;
}

int image_check(image_t *im)
{

  int i;
  int boot = -1;
  for(i = 0; i < 4; i++)
  {
    CHS_t chs;
    chs.C = ((im->mbr[i].start_sector & 0x3) << 8) + im->mbr[i].start_cylinder;
    chs.H = im->mbr[i].start_head;
    chs.S = (im->mbr[i].start_sector >> 2)&0x3F;
    if(LBA_from_CHS(im, chs) != im->mbr[i].start_LBA)
      return i+1;
    chs.C = ((im->mbr[i].end_sector & 0x3) << 8) + im->mbr[i].end_cylinder;
    chs.H = im->mbr[i].end_head;
    chs.S = (im->mbr[i].end_sector >> 2)&0x3F;
    if(LBA_from_CHS(im, chs) != im->mbr[i].num_sectors + im->mbr[i].start_LBA)
      return i+4;
  
    if((im->mbr[i].system_id & 0x80) == 0x80)
      boot = i;
  }

  if(boot == -1)
    return 9;

  return 0;
}

int image_readblocks(image_t *im, void *buffer, size_t start, size_t len)
{
  if(!im)
    return 0;

  fseek(im->file, start*BLOCK_SIZE, SEEK_SET);
  return fread(buffer, len*BLOCK_SIZE, 1, im->file);
}

int image_writeblocks(image_t *im, void *buffer, size_t start, size_t len)
{
  if (!im)
    return 0;

  fseek(im->file, start*BLOCK_SIZE, SEEK_SET);
  return fwrite(buffer, len*BLOCK_SIZE, 1, im->file);
}

CHS_t CHS_from_LBA(image_t *image, int lba)
{
  CHS_t ret;
  ret.C = lba/(image->heads*image->sectors);
  ret.H = (lba / image->sectors) % image->heads;
  ret.S = lba % image->sectors + 1;
  return ret;
}

size_t LBA_from_CHS(image_t *image, CHS_t chs)
{
  size_t ret = ((chs.C*image->heads)+chs.H)*image->sectors + chs.S -1;
  return ret;
}
