#include <dito.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void usage(const char *argv[])
{
  printf("usage: %s image partition output\n", argv[0]);
}

int main(int argc, const char *argv[])
{

  if(argc != 4)
  {
    usage(argv);
    return 1;
  }

  char *image_fn = strdup(argv[1]);
  image_t *im = image_load(image_fn);
  int partition = (int)argv[2][0]-(int)'0';

  MBR_entry_t *mbr = &im->mbr[partition];

  FILE *output = fopen(argv[3], "w+");

  unsigned int i;
  void *buffer = malloc(512);
  fseek(im->file, 512*mbr->start_LBA, SEEK_SET);
  for(i = 0; i < mbr->num_sectors; i++)
  {
    int readcount = fread(buffer, 1, 512, im->file);
    fwrite(buffer, 1, readcount, output);
  }
  printf("Extracted %d sectors starting at %d \n", i, mbr->start_LBA);

  return 0;
}
