#include <partition.h>
#include <image.h>
#include <stdlib.h>
#include <stdio.h>

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


int partition(image_t *image, size_t sizes[4], int boot)
{
  if(boot < 0 || boot > 3)
    return 1;
  if((sizes[0]+sizes[1]+sizes[2]+sizes[3]) > 
      image->cylinders*image->heads*image->sectors*BLOCK_SIZE)
    return 1;

  MBR_entry_t *MBR = calloc(4, sizeof(MBR_entry_t));
  size_t i = 0, pos=image->sectors;
  
  for(i = 0; i < 4; i++)
  {
    size_t round_size = (sizes[i]+(sizes[i]%BLOCK_SIZE!=0)*BLOCK_SIZE);
    round_size = round_size - round_size%BLOCK_SIZE;
    size_t start_LBA = pos/BLOCK_SIZE;
    size_t end_LBA = start_LBA + round_size/BLOCK_SIZE;
    CHS_t start = CHS_from_LBA(image, start_LBA);
    CHS_t end = CHS_from_LBA(image, end_LBA);
    
    MBR[i].boot_indicator = 0;
    MBR[i].start_head = start.H;
    MBR[i].start_sector = 
      ((start.S &0x3F) << 2) | ((start.C & 0x300) >> 8);
    MBR[i].start_cylinder = (start.C & 0xFF);
    MBR[i].system_id = (sizes[i])?0x83:0x0;
    MBR[i].end_head = end.H;
    MBR[i].end_sector = ((end.S & 0x3F) << 2) | ((end.C & 0x300) >> 8);
    MBR[i].end_cylinder = (end.C & 0xFF);
    MBR[i].start_LBA = start_LBA;
    MBR[i].num_sectors = round_size/BLOCK_SIZE;

    pos += round_size;
  }

  if(boot >=0 && boot <=3)
    MBR[boot].boot_indicator = 0x80;

  write_mbr(image, MBR);

  return 0;
}

int set_partition_system(image_t *image, int partition, uint8_t system)
{
  if(partition <0 || partition >3)
    return 1;

  MBR_entry_t *MBR = load_mbr(image);

  MBR[partition].system_id = system;

  write_mbr(image, MBR);

  return 0;
}

partition_t *get_partition(image_t *image, int partition)
{
  if(!image)
    return 0;
  if(!image->file)
    return 0;

  MBR_entry_t *mbr = load_mbr(image);

  partition_t *ret = malloc(sizeof(partition_t));
  ret->im = image;
  ret->partition = partition;
  ret->offset = mbr[partition].start_LBA*BLOCK_SIZE;
  ret->length = mbr[partition].num_sectors*BLOCK_SIZE;
  return ret;
}

void free_partition(partition_t *p)
{
  free(p);
}
