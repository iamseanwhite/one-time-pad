#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]){
	
	srand(time(NULL));
	
	if (argc != 2) { 									// Check usage & args
		fprintf(stderr,"USAGE: %s length\n", argv[0]); 
		exit(1); 
	} 
	
	
	int keyLength = atoi(argv[1]);
	char* letterArray = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	char* key = malloc(sizeof(char) * keyLength);
	
	for (int i = 0; i < keyLength; i++){
		int randomNumber = rand() % 27;
		key[i] = letterArray[randomNumber];	
	}
	
	printf("%s\n", key);
	
	free(key);
	
	return 0;
}