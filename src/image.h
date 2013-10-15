#pragma once

#include <stdio.h>
#include <stdint.h>

typedef struct
{
  char *filename;
  FILE *file;
} image_t;


#define BLOCK_SIZE 512
#define MBR_OFFSET 436

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


image_t *load_image(char *filename);

MBR_entry_t *load_mbr(image_t *image);


