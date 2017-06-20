#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

// Error function used for reporting issues
void error(const char *msg) {	
	perror(msg); 
	exit(1); 
} 

int main(int argc, char *argv[]) {
	
	int childExitStatus = -5;
	
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[140000];
	char partial[70000];
	struct sockaddr_in serverAddress, clientAddress;
	char* localhost = "localhost";
	pid_t spawnpid = -5;
	
	// Check usage & args
	if (argc < 2) { 
		fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); 
	} 


	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(atoi(argv[1])); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process


	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");


	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	while(1){
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr*) &clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0) error("ERROR on accept");
	
		spawnpid = fork();
		
		switch(spawnpid) {
			case -1: {
	        	//done goofed
		        printf("Something seems to have gone wrong. -__-");
		        exit(1);
		        break;
	        }
	                
	        case 0: {
	    	    //child
	    	
				// Get the message from the client and display it
				memset(buffer, '\0', 140000);
				while(strstr(buffer, "&&") == NULL){
					memset(partial, '\0', 70000);
					charsRead = recv(establishedConnectionFD, partial, 70000 - 1, 0); // Read the client's message from the socket
					strcat(buffer, partial);
					if (charsRead < 0) error("ERROR reading from socket");
				}
				
				//printf("SERVER: I received this from the client: \"%s\"\n", buffer);

				//reject if not from proper source
				int compareReturn = strncmp(buffer, "dec", 3);
				if (compareReturn != 0){  //if not from otp_enc
				
					char cyphertext[strlen(buffer)];
					memset(cyphertext, '\0', sizeof(cyphertext));
					cyphertext[0] = 'N';
					cyphertext[1] = 'O';
					
					// Send a Success message back to the client
					charsRead = send(establishedConnectionFD, cyphertext, sizeof(cyphertext), 0); // Send success back
					if (charsRead < 0) error("ERROR writing to socket");
					close(establishedConnectionFD); // Close the existing socket which is connected to the client
					
					break;
				}
				
				else {

					//strip "enc" and key from buffer
					char subbuffer[sizeof(buffer)];
					memcpy(subbuffer, &buffer[3], sizeof(buffer) - 3);
					subbuffer[sizeof(buffer) - 3] = '\0';
				
					memset(buffer, '\0', sizeof(buffer));
					strcpy(buffer, subbuffer);
					char plaintextPlusKey[sizeof(buffer)];
					strcpy(plaintextPlusKey, buffer);
					
				//	printf("New buffer after dec taken out: %s\n", plaintextPlusKey);

				//------------------------------------------------------------------------------
					int i;
					int positionOfColon;
					for (i = 0; i < sizeof(buffer); i++) {
						if (buffer[i] == ':') {
							positionOfColon = i;
							break;
						}
					}

					//extract key from full buffer
					char subbuffer2[sizeof(buffer)];
					memcpy(subbuffer2, &buffer[i+1], sizeof(buffer) - positionOfColon);
					subbuffer2[sizeof(buffer) - positionOfColon] = '\0';

					memset(buffer, '\0', sizeof(buffer));
					strcpy(buffer, subbuffer2);
					char key[sizeof(buffer)];
					strcpy(key, buffer);
					
					//printf("New key that has been extracyed is: %s\n", key);
					//printf("strlen of key is : %zu\n", strlen(key));

					//------------------------------------------------------------------------------
					plaintextPlusKey[i] = '\0';
					strcpy(buffer, plaintextPlusKey);
					//printf("The stripped plaintext is: %s\n", buffer);
				
				fflush(stdout);	
					//Do cryptomagic	
					char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
					int cypherNum[strlen(buffer)];
					int keyNum[strlen(buffer)];
					int plaintextNum[strlen(buffer)];
					char temp;
					int j;
					
					//build cypherNum array
					for (i = 0; i < strlen(buffer); i++) {
						temp = buffer[i];
						for (j = 0; j < strlen(alphabet); j++) {
							if (temp == alphabet[j]) {
								cypherNum[i] = j;
							}
						}
					}

					//build keyNum array
					for (i = 0; i < strlen(buffer); i++) {
						temp = key[i];
						for (j = 0; j < strlen(alphabet); j++) {
							if (temp == alphabet[j]) {
								keyNum[i] = j;
							}
						}
					}

					//build plaintextNum
					for (i = 0; i < strlen(buffer); i++) {
						plaintextNum[i] = cypherNum[i] - keyNum[i];
						if (plaintextNum[i] < 0) {					//% 27
							plaintextNum[i] += 27;
						}
					}

					//build plaintext string from int array
					char plaintext[strlen(buffer)];
					memset(plaintext, '\0', sizeof(plaintext));
					
					for (i = 0; i < strlen(buffer); i++) {
						
						for (j = 0; j < strlen(alphabet); j++) {
							
							if (plaintextNum[i] == j){
								plaintext[i] = alphabet[j];
							}
						}

					}

					
					// Send a Success message back to the client
					charsRead = send(establishedConnectionFD, plaintext, sizeof(plaintext), 0); // Send success back
					if (charsRead < 0) error("ERROR writing to socket");
					close(establishedConnectionFD); // Close the existing socket which is connected to the client
					
					
					break;
				}
			
				
	        	
	        }
		    default: {
	            //parent
	            pid_t actualPid = waitpid(spawnpid, &childExitStatus, 0);
	            break;
		    }
		}
	}
	close(listenSocketFD); // Close the listening socketchmod 
	return 0; 
}
