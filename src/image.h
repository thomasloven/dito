#pragma once

#include <stdio.h>
#include <stdint.h>
#include <dito.h>



#define BLOCK_SIZE 512
#define MBR_OFFSET 446

typedef struct
{
  uint32_t C;
  uint32_t H;
  uint32_t S;
} CHS_t;


MBR_entry_t *image_getmbr(image_t *im, int num);
void image_setmbr(image_t *im, MBR_entry_t *mbr, int num);
size_t image_getsize(image_t *im);
size_t image_get_partition_start(image_t *im, int num);
size_t image_get_partition_length(image_t *im, int num);

int image_check(image_t *im);
int image_readblocks(image_t *im, void *buffer, size_t start, size_t len);
int image_writeblocks(image_t *im, void *buffer, size_t start, size_t len);

CHS_t CHS_from_LBA(image_t *image, int lba);
size_t LBA_from_CHS(image_t *image, CHS_t chs);
