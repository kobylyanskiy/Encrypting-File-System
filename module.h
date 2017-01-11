#define EFS_MAGIC_NUMBER 0x11111111 
#define EFS_DEFAULT_BLOCK_SIZE 4096 
#define EFS_FILENAME_MAXLEN 255 
 
const int EFS_ROOT_INODE_NUMBER = 1; 
const int EFS_ROOTDIR_DATABLOCK_NUMBER = 2;

struct efs_dir_record { 
  char filename[EFS_FILENAME_MAXLEN]; 
  uint32_t inode_no; 
}; 
 
struct efs_inode { 
  mode_t mode; 
  uint32_t inode_no; 
  uint32_t data_block_number; 
 
  union { 
    uint32_t file_size; 
    uint32_t dir_children_count; 
  }; 
}; 

struct efs_super_block { 
	uint32_t version; 
	uint32_t magic; 
	uint32_t block_size; 
	uint32_t free_blocks;
	char padding[(4096) - (4 * sizeof(uint32_t))]; 
};

struct efs_free_blocks {
	char bitmap[4096];
};
