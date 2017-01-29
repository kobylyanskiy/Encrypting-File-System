#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "efs.h"

#define SB_BLOCK_NUMBER 0
#define INODE_BITMAP_BLOCK_NUMBER 1
#define DATA_BITMAP_BLOCK_NUMBER 2
#define INODE_TABLE_BLOCK_NUMBER 3

void sb_check(int fd){
	struct efs_super_block sb;
	ssize_t ret;
	
	ret = read(fd, &sb, EFS_DEFAULT_BLOCK_SIZE);
	if(ret != EFS_DEFAULT_BLOCK_SIZE){
		printf("There is an error while reading superblock\n");	
	}

	if(sb.magic == EFS_MAGIC_NUMBER){
		printf("EFS has been detected on disk\n");
	}
}

void inode_bitmap_check(int fd){
	int* inode_bitmap;	
	ssize_t ret;
	int i;	

	inode_bitmap = malloc(EFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED * sizeof(int));
	ret = read(fd, inode_bitmap, EFS_DEFAULT_BLOCK_SIZE);
	if(ret != EFS_DEFAULT_BLOCK_SIZE){
		printf("There is an error while reading inode bitmap\n");	
	}
	
	for(i = 0; i < EFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED; i++){
		if(inode_bitmap[i])
			printf("Inode with number %d is busy\n", i);
	}
	free(inode_bitmap);
}

void data_bitmap_check(fd){
	char* data_bitmap;	
	ssize_t ret;
	int i;	

	data_bitmap = malloc(EFS_DEFAULT_BLOCK_SIZE * sizeof(char));
	ret = read(fd, data_bitmap, EFS_DEFAULT_BLOCK_SIZE);
	if(ret != EFS_DEFAULT_BLOCK_SIZE){
		printf("There is an error while reading data bitmap\n");	
	}
	
	for(i = 0; i < EFS_DEFAULT_BLOCK_SIZE; i++){
		if(data_bitmap[i])
			printf("Block with number %d is busy\n", i);
	}
	free(data_bitmap);
}

void inode_table_check(fd){
	struct efs_inode* inode_block;	
	ssize_t ret;
	int i, j;	

	inode_block = malloc(EFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED / 32 * sizeof(struct efs_inode));
	
	for(i = 0; i < 32; i++){	
		ret = read(fd, inode_block, EFS_DEFAULT_BLOCK_SIZE);
		if(ret != EFS_DEFAULT_BLOCK_SIZE){
			printf("There is an error while reading %d inode block\n", i);	
		}
		for(j = 0; j < 32; j++){
			if(inode_block[j].data_block_number)
				printf("Inode with number %d is busy\n", j + j * i);
		}
			printf("Read %d inode group\n", i);	
	}
	free(inode_block);
}

int main(int argc, char *argv[]){
        int fd;

	if (argc != 2) {
                printf("Usage: efsck <device>\n");
                return -1;
        }

	fd = open(argv[1], O_RDONLY);
	sb_check(fd);
	inode_bitmap_check(fd);	
	data_bitmap_check(fd);	
	inode_table_check(fd);

	return 0;
}
