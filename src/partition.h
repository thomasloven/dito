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


partition_t *partition_open(image_t *im, int partition);

void partition_close(partition_t *p);
size_t partition_readblocks(partition_t *p, void *buffer, size_t start, size_t len);
size_t partition_writeblocks(partition_t *p, void *buffer, size_t start, size_t len);
