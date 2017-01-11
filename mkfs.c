#include <unistd.h> 
#include <stdio.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <stdint.h>
#include <fcntl.h> 
#include "module.h" 


static int write_root_inode(int fd){

	ssize_t ret;

	struct efs_inode root_inode = {
		.mode = S_IFDIR,
		.dir_children_count = 1
	};

	ret = write(fd, &root_inode, sizeof(root_inode));

	if (ret != sizeof(root_inode)) {
		printf
		    ("The inode store was not written properly. Retry your mkfs\n");
		return -1;
	}

	printf("root directory inode written succesfully\n");
	return 0;

}

static int write_superblock(int fd){

	ssize_t ret; 
	struct efs_super_block sb = {
		.version = 1,
		.magic = EFS_MAGIC_NUMBER,
		.block_size = EFS_DEFAULT_BLOCK_SIZE,
		.free_blocks = ~0
	};

	ret = write(fd, (char *)&sb, sizeof(sb));
	if (ret != EFS_DEFAULT_BLOCK_SIZE) {
		printf
		    ("bytes written [%d] are not equal to the default block size\n",
		     (int)ret);
		return -1;
	} 
	
	printf("Super block written succesfully\n");
	return 0;

}

int main(int argc, char *argv[]) { 
	int fd; 
	
	if (argc != 2) { 
	  printf("Usage: mkfs <device>\n"); 
	  return -1; 
	} 
	
	fd = open(argv[1], O_RDWR); 
	if (fd == -1) { 
	  perror("Error opening the device\n"); 
	  return -1; 
	} 

	write_superblock(fd);
	write_root_inode(fd);	
	
	
	close(fd); 
	
	return 0; 
} 
