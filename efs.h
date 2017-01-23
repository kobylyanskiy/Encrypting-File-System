#define EFS_MAGIC_NUMBER 0x11111111 
#define EFS_DEFAULT_BLOCK_SIZE 4096
#define EFS_FILENAME_MAXLEN 255 
 
const int EFS_ROOTDIR_INODE_NUMBER = 1; 
const int EFS_ROOTDIR_DATABLOCK_NUMBER = 2;
const int EFS_SUPERBLOCK_BLOCK_NUMBER = 0;
const int EFS_INODESTORE_BLOCK_NUMBER = 1;
const int EFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED = 64;

struct efs_dir_record { 
  char filename[EFS_FILENAME_MAXLEN]; 
  uint64_t inode_no; 
}; 
 
struct efs_inode { 
  mode_t mode; 
  uint64_t inode_no; 
  uint64_t data_block_number; 
 
  union { 
    uint64_t file_size; 
    uint64_t dir_children_count; 
  }; 
}; 

struct efs_super_block { 
	uint64_t version; 
	uint64_t magic; 
	uint64_t block_size; 
	uint64_t inodes_count;
	uint64_t free_blocks;
	char padding[(4096) - (5 * sizeof(uint64_t))]; 
};
