#pragma once
#include <stdint.h>

typedef struct fat_bpb_st
{
  uint8_t jmp[3];
  uint8_t identifier[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fat_count;
  uint16_t root_count;
  uint16_t total_sectors_small;
  uint8_t media_descriptor;
  uint16_t sectors_per_fat;
  uint16_t sectors_per_track;
  uint16_t num_heads;
  uint32_t hidden_sectors;
  uint32_t total_sectors_large;
  union
  {
    struct fat16
    {
      uint8_t drive;
      uint8_t flags;
      uint8_t signature;
      uint32_t volume_id;
      uint8_t volume_label[11];
      uint8_t system_id[8];
      uint8_t unused[448];
      uint8_t boot_signature[2];
    };
    struct fat32
    {
      uint32_t sectors_per_fat;
      uint16_t flags;
      uint16_t fat_version;
      uint32_t root_cluster;
      uint16_t fsinfo_cluster;
      uint16_t boot_backup_cluster;
      uint8_t reserved[12];
      uint8_t drive;
      uint8_t flags;
      uint8_t signature;
      uint32_t volume_id;
      uint8_t volume_label[11];
      uint8_t system_id[8];
      uint8_t unused[420];
      uint8_t boot_signature[2];
    };
  }
} fat_bpb_t;

typedef struct fat_dir_st
{
  uint8_t name[11];
  uint8_t attrib;
  uint8_t reserved;
  uint8_t c_sec;
  uint16_t c_time;
  uint16_t c_date;
  uint16_t a_date;
  uint16_t cluster_high;
  uint16_t m_time;
  uint16_t m_date;
  uint16_t cluster_low;
  uint32_t size;
} fat_dir_t;

#define FAT_DIR_READ_ONLY 0x01
#define FAT_DIR_HIDDEN 0x02
#define FAT_DIR_SYSTEM 0x04
#define FAT_DIR_VOLUME_ID 0x08
#define FAT_DIR_DIRECTORY 0x10
#define FAT_DIR_ARCHIVE 0x20
#define FAT_DIR_LONGNAME 0x0F

typedef struct fat_longname_st
{
  uint8_t num;
  uint8_t name1[10];
  uint8_t attrib;
  uint8_t entry_type;
  uint8_t checksum;
  uint8_t name2[12];
  uint16_t zero;
  uint8_t name3[4];
} fat_longname_t;

typedef struct
{
  fat_bpb_t *bpb;
} fat_data_t;

#define fat_data(fs) ((fat_data->t *)(fs)->data)
#define fat_bpb(fs) ((fat_data((fs)))->bpb)
#define fat_root_sectors(fs) (((fat_bpb(fs)->root_count * 32) + (fat_bpb(fs)->bytes_per_sector - 1)) / fat_bpb(fs)->bytes_per_sector)
#define fat_first_data_sector(fs) (fat_bpb(fs)->reserved_sectors + (fat_bpb(fs)->fat_count * fat_bpb(fs)->sectors_per_fat))
#define fat_fat_start(fs) (fat_bpb(fs)->reserved_sectors)
#define fat_num_sectors(fs) (fat_bpb(fs)) // UNFINISHED
