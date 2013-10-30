#pragma once

#include <stdint.h>
#include <dito.h>
#include "image.h"

#define MBR_OFFSET 446

size_t partition_readblocks(partition_t *p, void *buffer, size_t start, size_t len);
size_t partition_writeblocks(partition_t *p, void *buffer, size_t start, size_t len);
