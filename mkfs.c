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

	printf("root directory inode written succesfully, [%ld] bytes was written\n", ret);
	return 0;

}

static int write_free_blocks(int fd){
	ssize_t ret;

	char bitmap[4096] = {0};
	bitmap[0] = 124;

	struct efs_free_blocks free_blocks = {
		.bitmap = *bitmap
	};

	ret = write(fd, &free_blocks, sizeof(free_blocks));

	if (ret != sizeof(free_blocks)) {
		printf
		    ("Free blocks were not written properly. Retry your mkfs\n");
		return -1;
	}

	printf("free blocks written succesfully, [%ld] bytes was written\n", ret);
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
		    ("[%ld] bytes written aren't equal to the default block size\n", ret);
		return -1;
	} 
	
	printf("Super block written succesfully, [%ld] bytes was written\n", ret);
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
	write_free_blocks(fd);
	write_root_inode(fd);	
	
	
	close(fd); 
	
	return 0; 
} 
