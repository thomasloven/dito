#pragma once

#include <stdio.h>
#include <stdint.h>

typedef struct
{
  char *filename;
  FILE *file;
  size_t cylinders;
  size_t heads;
  size_t sectors;
} image_t;


#define BLOCK_SIZE 512

typedef struct
{
  int C;
  int H;
  int S;
} CHS_t;

image_t *new_image(char *filename, size_t size);
image_t *load_image(char *filename);
void free_image(image_t *image);

CHS_t CHS_from_LBA(image_t *image, int lba);
size_t LBA_from_CHS(image_t *image, CHS_t chs);
