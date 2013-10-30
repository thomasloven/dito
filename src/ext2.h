#pragma once

#include <time.h>
#include <stdint.h>
#include <fs.h>

typedef struct // Superblock
{
  uint32_t num_inodes;
  uint32_t num_blocks;
  uint32_t num_reserved_blocks;
  uint32_t num_free_blocks;
  uint32_t num_free_inodes;
  uint32_t superblock_block;
  uint32_t block_size;
  uint32_t fragment_size;
  uint32_t blocks_per_group;
  uint32_t fragments_per_group;
  uint32_t inodes_per_group;
  int32_t last_mount_time;
  int32_t last_write_time;
  uint16_t mount_count;
  uint16_t max_mount_count;
  uint16_t signature;
  uint16_t fs_state;
  uint16_t error_method;
  uint16_t version_minor;
  uint32_t last_check_time;
  uint32_t check_interval;
  uint32_t operating_system;
  uint32_t version_major;
  uint16_t uid;
  uint16_t gid;

  // Extended superblock fields
  uint32_t first_inode;
  uint16_t inode_size;
  uint16_t superblock_group;
  uint32_t optional_features;
  uint32_t required_features;
  uint32_t readwrite_features;
  char fs_id[16];
  char volume_name[16];
  char last_path[64];
  uint32_t compression;
  uint8_t file_start_blocks;
  uint8_t dir_start_blocks;
  char unused[2];
  char journal_id[16];
  uint32_t journal_inode;
  uint32_t journal_device;
  uint32_t orphan_inodes_head;
}__attribute__((packed)) ext2_superblock_t;

#define EXT2_SUPERBLOCK_SIZE 1024

typedef struct // group
{
  uint32_t block_bitmap;
  uint32_t inode_bitmap;
  uint32_t inode_table;
  uint16_t unallocated_blocks;
  uint16_t unallocated_inodes;
  uint16_t num_dir;
  uint16_t unused[7];
}__attribute__((packed)) ext2_groupd_t;

typedef struct // inode
{
  uint16_t type;
  uint16_t uid;
  uint32_t size_low;
  uint32_t atime;
  uint32_t ctime;
  uint32_t mtime;
  uint32_t dtime;
  uint16_t gid;
  uint16_t link_count;
  uint32_t disk_sectors;
  uint32_t flags;
  uint32_t osval1;
  uint32_t direct[12];
  uint32_t indirect;
  uint32_t dindirect;
  uint32_t tindirect;
  uint32_t generation;
  uint32_t extended_attributes;
  uint32_t size_high;
  uint32_t fragment_block;
  uint8_t osval2[12];
}__attribute__((packed)) ext2_inode_t;

#define EXT2_FIFO 0x1000
#define EXT2_CHDEV 0x2000
#define EXT2_DIR 0x4000
#define EXT2_BDEV 0x6000
#define EXT2_REGULAR 0x8000
#define EXT2_SYMLINK 0xA000
#define EXT2_SOCKET 0xC000
#define EXT2_OX 00001
#define EXT2_OW 00002
#define EXT2_OR 00004
#define EXT2_GX 00010
#define EXT2_GW 00020
#define EXT2_GR 00040
#define EXT2_UX 00100
#define EXT2_UW 00200
#define EXT2_UR 00400


typedef struct // dirinfo
{
  uint32_t inode;
  uint16_t record_length;
  uint8_t name_length;
  uint8_t file_type;
  char name[1];
}__attribute__((packed)) ext2_dirinfo_t;

#define EXT2_DIR_UNKNOWN 0
#define EXT2_DIR_REGULAR 1
#define EXT2_DIR_DIR 2
#define EXT2_DIR_CHDEV 3
#define EXT2_DIR_BDEV 4
#define EXT2_DIR_FIFO 5
#define EXT2_DIR_SOCKET 6
#define EXT2_DIR_SYMLINK 7


typedef struct 
{
  ext2_superblock_t *superblock;
  int superblock_dirty;
  ext2_groupd_t *groups;
  unsigned int num_groups;
  int groups_dirty;
  
  ext2_inode_t ino_buffer;
  uint32_t buffer_inode;
  int buffer_dirty;
} ext2_data_t;

#define ext2_blocksize(fs) (1024 << ((ext2_data_t *)(fs)->data)->superblock->block_size)
#define ext2_numgroups(fs) ((((ext2_data_t *)(fs)->data)->superblock->num_inodes / ((ext2_data_t *)(fs)->data)->superblock->inodes_per_group) + (((ext2_data_t *)(fs)->data)->superblock->num_inodes % ((ext2_data_t *)(fs)->data)->superblock->inodes_per_group != 0))
  


int ext2_read(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset);
int ext2_write(struct fs_st *fs, INODE ino, void *buffer, size_t length, size_t offset);
INODE ext2_touch(struct fs_st *fs, fstat_t *st);
dirent_t *ext2_readdir(struct fs_st *fs, INODE dir, unsigned int num);
int ext2_link(struct fs_st *fs, INODE ino, INODE dir, const char *name);
int ext2_unlink(struct fs_st *fs, INODE dir, unsigned int num);
fstat_t *ext2_fstat(struct fs_st *fs, INODE ino);
int ext2_mkdir(struct fs_st *fs, INODE parent, const char *name);
INODE root;

void *ext2_hook_load(struct fs_st *fs);
void *ext2_hook_create(struct fs_st *fs);
void ext2_hook_close(struct fs_st *fs);
int ext2_hook_check(struct fs_st *fs);

fs_driver_t ext2_driver;
uint32_t *ext2_get_blocks(fs_t *fs, ext2_inode_t *node);
int ext2_read_inode(struct fs_st *fs, ext2_inode_t *buffer, int num);
int ext2_readblocks(struct fs_st *fs, void *buffer, size_t start, size_t len);
int ext2_writeblocks(struct fs_st *fs, void *buffer, size_t start, size_t len);
