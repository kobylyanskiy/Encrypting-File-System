#include <stdio.h> 
#include <stdlib.h>
#include <openssl/aes.h>
#include <openssl/evp.h>


size_t calculate_size(FILE* input_file){
	int filesize;
	fseek(input_file, 0, SEEK_END);
	filesize = ftell(input_file);
	rewind(input_file);
	return filesize;
}

FILE* open_file(char* filename){
	FILE* input_file;
	
	input_file = fopen(filename, "r+b");

	if(input_file == NULL){
		printf("Cannot open file\n");
		exit(1);
	}
	return input_file;
}

void encrypt_decrypt(char* filename, char* passkey){

	int num = 0;
	AES_KEY key;
	AES_set_encrypt_key(passkey, 128, &key);

	FILE* file = open_file(filename); 
	size_t size = calculate_size(file);

	char* input = (char*)malloc(size);
	char* encrypted = (char*)malloc(size);
	
	fread(input, 1, size, file);

	file = fopen(filename, "w+b");

	AES_cfb128_encrypt(input, encrypted, size, &key, passkey, &num, AES_ENCRYPT);
	
	fwrite(encrypted, size, 1, file);
	fclose(file);

	printf("%s was successfully encrypted/decrypted\n", filename);
}

int main (int argc, char *argv[]) {

	char passkey[256];
	char filename[256];

	printf("Please, choose, which file you want to encrypt/decrypt: ");
	scanf("%s", filename);

	printf("Please, enter passkey: ");
	scanf("%s", passkey);

	encrypt_decrypt(filename, passkey);
	return 0;
}

