#pragma once

#include <stdio.h>
#include <stdint.h>

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
  char *filename;
  FILE *file;
  size_t cylinders;
  size_t heads;
  size_t sectors;
  MBR_entry_t mbr[4];
  int mbr_dirty;

} image_t;


#define BLOCK_SIZE 512
#define MBR_OFFSET 446

typedef struct
{
  int C;
  int H;
  int S;
} CHS_t;

image_t *image_new(char *filename, size_t sizes[4], int boot);
image_t *image_load(char *filename);
void image_close(image_t *im);

MBR_entry_t *image_getmbr(image_t *im, int num);
void image_setmbr(image_t *im, MBR_entry_t *mbr, int num);
size_t image_getsize(image_t *im);

int image_check(image_t *im);
int image_readblocks(image_t *im, void *buffer, size_t start, size_t len);
int image_writeblocks(image_t *im, void *buffer, size_t start, size_t len);

CHS_t CHS_from_LBA(image_t *image, int lba);
size_t LBA_from_CHS(image_t *image, CHS_t chs);
