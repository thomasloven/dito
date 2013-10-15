#pragma once

#include <stdint.h>
#include <image.h>

#define MBR_OFFSET 446

typedef struct
{
  uint8_t boot_indicator;
  uint8_t start_head;
  uint8_t start_sector;
  uint8_t start_cylinder;
  uint8_t system_id;
  uint8_t end_head;
  uint8_t end_sector;
  uint8_t end_cylinder;
  uint32_t start_LBA;
  uint32_t num_sectors;
}__attribute__((packed)) MBR_entry_t;

typedef struct
{
  image_t *im;
  int partition;
  size_t offset;
  size_t length;
} partition_t;


MBR_entry_t *load_mbr(image_t *image);
void write_mbr(image_t *image, MBR_entry_t *MBR);
int partition(image_t *image, size_t sizes[4], int boot);
int set_partition_system(image_t *image, int partition, uint8_t system);

partition_t *get_partition(image_t *image, int partition);
void free_partition(partition_t *p);

size_t read(partition_t *p, void *buffer, size_t bytes, size_t offset);
size_t write(partition_t *p, const void *buffer, size_t bytes, size_t offset);
