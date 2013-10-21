#pragma once

#include <stdint.h>
#include <image.h>

#define MBR_OFFSET 446

typedef struct
{
  image_t *im;
  int partition;
  size_t offset;
  size_t length;
} partition_t;


partition_t *partiton_load(image_t *image, int partition);

void partition_close(partition_t *p);
void partition_readblocks(partition_t *p, void *buffer, size_t start, size_t len);
void partition_writeblocks(partition_t *p, void *buffer, size_t start, size_t len);
