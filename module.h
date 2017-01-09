static unsigned long const EFS_MAGIC_NUMBER = 0x1488228;

const int EFS_DEFAULT_BLOCK_SIZE = 4 * 1024; 
 
struct efs_super_block { 
  unsigned int version; 
  unsigned int magic; 
  unsigned int block_size; 
  unsigned int free_blocks; 
 
  char padding[ (4 * 1024) - (4 * sizeof(unsigned int))]; 
};
