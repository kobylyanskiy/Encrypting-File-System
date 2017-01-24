#include <stdio.h> 
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

size_t calculate_size(int fd){
	struct stat st;
	fstat(fd, &st);
	return st.st_size;	
}

int open_file(char* filename){

	int fd;

	fd = open(filename, O_RDWR);
        if (fd == -1) {
		perror("Error opening the device");
                return -1;
        }

	return fd;
}

void encrypt_decrypt(char* filename){
	
	long position;
	char key[] = {'n', 'w', 'W', 'Z', 'T', 'M', '5', 'J', 'S', 'z', 'a',
	'L', 'D', '0', 'W', 't', 'Q', '8', 'd', 'G', 'w', 'I', 'n', 'O', 'l',
	'H', 'f', 'U', 'h', 'd', '7', 'z', 'Y', '2', '6', 'Z', '8', 'O', 'K',
	'N', 'F', 'M', 'l', 'k', 'J', 'O', 'n', 'C', 'r', '7', 'I', '8', 'Q',
	'K', 'u', '5', '7', '5', '2', 'H', 'y', 'V', 'P', 'g'};

	int fd = open_file(filename); 
	size_t size = calculate_size(fd);

	printf("opened file with the size of %ld bytes", size);

	char *input = (char*)malloc(size * sizeof(char));
	read(fd, input, size);

	char encrypted[size];
	
	int i;
	for(i = 0; i < size; i++)
		encrypted[i] = input[i] ^ key[i % (sizeof(key)/sizeof(char))];
	position = lseek(fd, 0L, SEEK_SET);
	printf("position: %ld\n", position);
	write(fd, encrypted, sizeof(encrypted));

	printf("%s was successfully encrypted/decrypted\n", filename);

}

int main (int argc, char *argv[]) {

	char filename[256];
	int flag;

	printf("Please, choose, which file you want to encrypt/decrypt: ");
	scanf("%s", filename);
	encrypt_decrypt(filename);

	return 0;
}
