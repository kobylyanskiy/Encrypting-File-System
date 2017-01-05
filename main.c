#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t calculate_size(FILE* input_file){
	fseek(input_file, 0, SEEK_END);
	return ftell(input_file);
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

void encrypt_decrypt(char* filename){
	
	char key[] = {'n', 'w', 'W', 'Z', 'T', 'M', '5', 'J', 'S', 'z', 'a',
	'L', 'D', '0', 'W', 't', 'Q', '8', 'd', 'G', 'w', 'I', 'n', 'O', 'l',
	'H', 'f', 'U', 'h', 'd', '7', 'z', 'Y', '2', '6', 'Z', '8', 'O', 'K',
	'N', 'F', 'M', 'l', 'k', 'J', 'O', 'n', 'C', 'r', '7', 'I', '8', 'Q',
	'K', 'u', '5', '7', '5', '2', 'H', 'y', 'V', 'P', 'g'};

	FILE* file = open_file(filename); 
	size_t size = calculate_size(file);
	rewind(file);

	char *input = (char*)malloc(size * sizeof(char));
	fread(input, sizeof(char), size, file);

	file = fopen(filename, "w+b");

	char encrypted[size];
	
	int i;
	for(i = 0; i < size; i++)
		encrypted[i] = input[i] ^ key[i % (sizeof(key)/sizeof(char))];

	fwrite(encrypted, sizeof(encrypted), 1, file);
	fclose(file);
}

int main (int argc, char *argv[]) {
	
//	char filename[] = "test";
	encrypt_decrypt(argv[1]);
	
	return 0;
}

