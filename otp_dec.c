#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h> 

void error(const char *msg) {    // Error function used for reporting issues
	perror(msg); 
	exit(0); 
} 

extern int errno;

int main(int argc, char *argv[])
{
	int errnum;
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char* cyphertextBuffer = NULL;
	char* keyFileBuffer = NULL;
	//char plaintextBuffer[256];
	char* localhost = "localhost";
	size_t length = 0;
    
	
	// Check usage & args
	if (argc < 4) { 												
		fprintf(stderr,"USAGE: %s plaintext key port\n", argv[0]); 
		exit(0); 
	} 
	
	
	// Check if both files are readable-----------------------------------------
	int plaintextFD = open(argv[1], O_RDONLY);
	if ( plaintextFD < 0) {
		fprintf(stderr,"Cannot read file\n"); 
		exit(1); 
	}
	close(plaintextFD);
	
	int keyFD = open(argv[2], O_RDONLY);
	if ( keyFD < 0) {
		fprintf(stderr,"Cannot read key file\n"); 
		exit(1); 
	}
	close(keyFD);
	
	
	//Open files and make sure keyfile is long enough---------------------------
	FILE* plaintextFile = fopen(argv[1], "r+");
	
	if (plaintextFile == NULL) {
		errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error opening file: %s\n", strerror( errnum )); 
		exit(1); 
	}
	
	FILE* keyFile = fopen(argv[2], "r+");
	
	if (keyFile == NULL) {
		fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error opening file: %s\n", strerror( errnum )); 
		exit(1); 
	}
	
	int plaintextLength = getline(&cyphertextBuffer, &length, plaintextFile);
	int keyFileLength = getline(&keyFileBuffer, &length, keyFile);
	
	//Remove newline from keyfile
	char* newline;
	if ((newline=strchr(keyFileBuffer, '\n')) != NULL)
		*newline = '\0';
	keyFileLength = strlen(keyFileBuffer);

	
	//Remove newline from plaintext file
	if ((newline=strchr(cyphertextBuffer, '\n')) != NULL)
		*newline = '\0';
	
	plaintextLength = strlen(cyphertextBuffer);
	
	//create 3 letter code and then concatenation all other info onto it. This will be the variable that is sent
	char secretCode[plaintextLength + 6 + keyFileLength];
	strcpy(secretCode,"dec");
	strcat(secretCode, cyphertextBuffer);
	strcat(secretCode, ":");
	strcat(secretCode, keyFileBuffer);
	strcat(secretCode, "&&");

	//this is the variable for recieving back from the server
	char plaintextBuffer[strlen(cyphertextBuffer)+1];
	
	
	//check key length
	if (keyFileLength < plaintextLength) {
		fprintf(stderr, "%s", "Error: key is too short\n");
		exit(1);
	}
	

	
	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname(localhost); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

		
	// Send message to server
	charsWritten = send(socketFD, secretCode, strlen(secretCode), 0); // Write to the server
	if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(secretCode)) printf("CLIENT: WARNING: Not all data written to socket!\n");
	
	fclose(plaintextFile);
	fclose(keyFile);

	// Get return message from server
	memset(plaintextBuffer, '\0', sizeof(plaintextBuffer)); // Clear out the cyphertextBuffer again for reuse
	charsRead = recv(socketFD, plaintextBuffer, sizeof(plaintextBuffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	if (plaintextBuffer[0] == 'N' && plaintextBuffer[1] == 'O') {
		fprintf(stderr, "%s", "Error: could not contact otp_enc_d - access denied\n");
		exit(2);
	}
	printf("%s", plaintextBuffer);
	printf("\n");

	close(socketFD); // Close the socket
	
	return 0;
}
