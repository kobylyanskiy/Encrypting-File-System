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

int main(int argc, char *argv[]){
	int fd, i;
	mode_t mode;
	ssize_t ret;
	struct timespec time;
	struct efs_super_block sb;
	struct efs_inode root_inode;
	struct efs_inode* inode_group;
	struct efs_inode welcomefile_inode;

	char* data_bitmap;
	int* inode_bitmap;

	struct efs_dir_record record;

	if (argc != 2) {
		printf("Usage: mkfs <device>\n");
		return -1;
	}

	fd = open(argv[1], O_RDWR);
	if (fd == -1) {
		perror("Error opening the device");
		return -1;
	}


	/* Begin writing of Block 0 - Super Block */
	sb.version = 1;
	sb.magic = EFS_MAGIC_NUMBER;
	sb.block_size = EFS_DEFAULT_BLOCK_SIZE;
	sb.inodes_count = 1;
	sb.free_blocks = 4096 - 7;

	ret = write(fd, (char *)&sb, sizeof(sb));

	if (ret != EFS_DEFAULT_BLOCK_SIZE) {
		printf
		    ("[%d] bytes written are not equal to the default block size\n",
		     (int)ret);
		ret = -1;
		goto exit;
	}

	printf("Superblock has been written succesfully\n");
	/* End of writing block 0 - superblock */


	/* Begin if writing block 1 - inode bitmap */
	inode_bitmap = calloc(EFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED, sizeof(int));	
	inode_bitmap[0] = 1;
	ret = write(fd, inode_bitmap, sizeof(int) * EFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED);

	if (ret != EFS_DEFAULT_BLOCK_SIZE) {
		printf
		    ("[%d] bytes written are not equal to the default block size\n",
		     (int)ret);
		ret = -1;
		goto exit;
	}
	free(inode_bitmap);
	printf("Inode table has been written succesfully\n");
	/* End of writing block 1 - inode bitmap */


	/* Begin if writing block 2 - data bitmap */
	data_bitmap = calloc(EFS_DEFAULT_BLOCK_SIZE, sizeof(char));	
	for(i = 0; i < EFS_ROOTDIR_DATABLOCK_NUMBER; i++){ 
		data_bitmap[i] = 1;
	}
	ret = write(fd, data_bitmap, sizeof(char) * EFS_DEFAULT_BLOCK_SIZE);

	if (ret != EFS_DEFAULT_BLOCK_SIZE) {
		printf
		    ("[%d] bytes written are not equal to the default block size\n",
		     (int)ret);
		ret = -1;
		goto exit;
	}
	free(data_bitmap);
	printf("Data table has been written succesfully\n");
	/* End of writing block 2 - data bitmap */


	/* Begin of writing blocks 3-34 - inode Store */
	
	inode_group = calloc(EFS_INODES_IN_BLOCK, sizeof(struct efs_inode));

	inode_group[0].mode = S_IFDIR;
	inode_group[0].inode_no = EFS_ROOTDIR_INODE_NUMBER;
	inode_group[0].data_block_number = EFS_ROOTDIR_DATABLOCK_NUMBER;
	inode_group[0].dir_children_count = 1;
	inode_group[0].uid = getuid();
	inode_group[0].gid = getgid();

	ret = (ssize_t)clock_gettime(CLOCK_REALTIME, &time);
	if(ret == -1){
		goto exit;
	}

	inode_group[0].atime = inode_group[0].mtime = inode_group[0].ctime = time;
	ret = write(fd, inode_group, EFS_DEFAULT_BLOCK_SIZE);

	if (ret != EFS_DEFAULT_BLOCK_SIZE) {
		printf
		    ("The inode store was not written properly. Retry your mkfs\n");
		ret = -1;
		goto exit;
	}
	
	inode_group = calloc(32, sizeof(struct efs_inode));
	for(i = 0; i < 31; i++){
		ret = write(fd, inode_group, EFS_DEFAULT_BLOCK_SIZE);
	} 

	printf("root directory inode written succesfully\n");
	/* End of writing blocks 3-34 - inode Store */
	ret = 0;

exit:
	close(fd);
	return ret;
}
