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

	printf("Data table has been written succesfully\n");
	/* End of writing block 2 - data bitmap */


	/* Begin of writing blocks 3-35 - inode Store */
	root_inode.mode = S_IFDIR;
	root_inode.inode_no = EFS_ROOTDIR_INODE_NUMBER;
	root_inode.data_block_number = EFS_ROOTDIR_DATABLOCK_NUMBER;
	root_inode.dir_children_count = 1;
	root_inode.uid = getuid();
	root_inode.gid = getgid();

	ret = (ssize_t)clock_gettime(CLOCK_REALTIME, &time);
	if(ret == -1){
		goto exit;
	}


	root_inode.atime = root_inode.mtime = root_inode.ctime = time;
	ret = write(fd, (char *)&root_inode, sizeof(root_inode));

	if (ret != sizeof(root_inode)) {
		printf
		    ("The inode store was not written properly. Retry your mkfs\n");
		ret = -1;
		goto exit;
	}
	printf("root directory inode written succesfully\n");
	/* End of writing blocks 3-35 - inode Store */
	ret = 0;

exit:
	close(fd);
	return ret;
}
