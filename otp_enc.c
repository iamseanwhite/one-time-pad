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
	char* plaintextBuffer = NULL;
	char* keyFileBuffer = NULL;
	//char cyphertextBuffer[256];
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
		
	int plaintextLength = getline(&plaintextBuffer, &length, plaintextFile);
	int keyFileLength = getline(&keyFileBuffer, &length, keyFile);
	
	//Remove newline from keyfile
	char* newline;
	if ((newline=strchr(keyFileBuffer, '\n')) != NULL)
		*newline = '\0';
	keyFileLength = strlen(keyFileBuffer);

		
	//Remove newline from plaintext file

	if ((newline=strchr(plaintextBuffer, '\n')) != NULL)
		*newline = '\0';
	plaintextLength = strlen(plaintextBuffer);
	
	//Check for bad characters
	char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	int i;
	int a;
	int matchfound = 0;
	
	for (i = 0; i < strlen(plaintextBuffer); i++) {
		for (a = 0; a < strlen(alphabet); a++) {
			if (plaintextBuffer[i] == alphabet[a]) {
				matchfound = 1;
			}
		}
		if (!matchfound) {
			fprintf(stderr, "%s", "otp_enc error: input contains bad characters \n");
				exit(1);
		}
	}
	
	//create 3 letter code and then concatenation all other info onto it. This will be the variable that is sent
	char secretCode[plaintextLength + 6 + keyFileLength];
	strcpy(secretCode,"enc");	
	strcat(secretCode, plaintextBuffer);
	strcat(secretCode, ":");
	strcat(secretCode, keyFileBuffer);
	strcat(secretCode, "&&");
		
	//this is the variable for recieving back from the server
	char cyphertextBuffer[strlen(plaintextBuffer)+1];
		
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
	memset(cyphertextBuffer, '\0', sizeof(cyphertextBuffer)); // Clear out the plaintextBuffer again for reuse

	charsRead = recv(socketFD, cyphertextBuffer, sizeof(cyphertextBuffer) - 1, 0); // Read data from the socket, leaving \0 at end

	if(charsRead < strlen(cyphertextBuffer) - 1){
		printf("Not all data came through");
	}
		
	fflush(stdout);
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	if (cyphertextBuffer[0] == 'N' && cyphertextBuffer[1] == 'O') {
		fprintf(stderr, "%s", "Error: could not contact otp_dec_d - access denied\n");
		exit(2);
	}
	
	printf("%s\n", cyphertextBuffer);

	fflush(stdout);

	close(socketFD); // Close the socket
	
	return 0;
}
