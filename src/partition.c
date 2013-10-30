#include "partition.h"
#include "image.h"
#include <dito.h>
#include <stdlib.h>
#include <stdio.h>


partition_t *partition_open(image_t *im, int partition)
{
  partition_t *p = malloc(sizeof(partition_t));
  p->im = im;
  p->partition = partition;
  p->offset = image_get_partition_start(im, partition);
  p->length = image_get_partition_length(im, partition);
  if(p->offset == 0 || p->length == 0)
  {
    free(p);
    return 0;
  }

  return p;
}

void partition_close(partition_t *p)
{
  if(!p)
    return;
  free(p);
}

size_t partition_readblocks(partition_t *p, void *buffer, size_t start, size_t len)
{
  if(!p)
    return 0;
  if(len > p->length)
    return 0;
  return image_readblocks(p->im, buffer, start + p->offset, len);
}

size_t partition_writeblocks(partition_t *p, void *buffer, size_t start, size_t len)
{
  if(!p)
    return 0;
  if(len > p->length)
    return 0;
  return image_writeblocks(p->im, buffer, start + p->offset, len);
}
