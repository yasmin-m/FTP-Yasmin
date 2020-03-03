#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>

#define MAX 1024 //max buffer sizes

int main(int argc, char *argv[]) 
{ 
	if (argc != 3){ //if less or more than 3 arguments entered
		printf("USAGE: ./FTPclient ftpserver-ip-address ftpserver-port-number\n");
	}

	int sockfd; //socket number
	int status; //temporarily stores status messages
	struct sockaddr_in serverAddress; //server address structure
	char message[MAX]; //buffer stores messages o be sent to server
	int PORT = atoi(argv[2]); //set port number to third argument
	if (PORT == 0){ //ATOI returns zero if it fails
		printf("USAGE: ./FTPclient ftpserver-ip-address ftpserver-port-number\n");
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0); //Create socket 
	if (sockfd == -1) { //If socket creation fails, print error
		printf("ERROR: SOCKET CREATION FAILED\n"); 
		exit(0); 
	}
 
	serverAddress.sin_family = AF_INET; //server connection setup 
	serverAddress.sin_addr.s_addr = inet_addr(argv[1]); 
	serverAddress.sin_port = htons(PORT); 

	status = connect(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
	if (status != 0) { //if connection fails, print error
		printf("ERROR: CONNECTION FAILED\n"); 
		exit(0); 
	}

	printf("CONNECTION ESTABLISHED TO SERVER\n"); //connection made

	 
	for (;;) { //loop over this block, waiting for commands
		memset(message, 0, sizeof(message)); //clear buffer that holds user input
		printf("ftp> "); //print ftp> at beginning to indicate new command

		fgets(message, MAX, stdin); //get user command

		if ((strncmp(message, "QUIT", 4)) == 0) { //if the user command is quit, quit
			printf("GOOD BYE!\n"); 
			break; 
		}

		if ((strncmp(message, "!PWD", 4)) == 0){ //if !PWD is entered
			char cwd[MAX];
			getcwd(cwd, sizeof(cwd)); //get current working directory
			printf("%s\n",cwd); //print current working directory
			continue; //go into next loop and wait for user command
		}

		if ((strncmp(message, "!CD ", 4)) == 0){ //if cd is entered
			char cwd[MAX];
			char dir[MAX];
			int size = strlen(message)-5;
			memset(dir, 0, MAX);

			int c = 0;
			while (c < size){   //iterate over the message to get the directory
				dir[c] = message[c+4];
				c++;
			}
			if (chdir(dir) == 0){ //if changed directory worked, print current working directory
				getcwd(cwd, sizeof(cwd));
				printf("%s\n",cwd);
			}
			else{ //else print error message
				printf("Command failed: No such file or directory \n");
			}
			continue;
		}

		if((strncmp(message, "!LS", 3)) == 0){ //if ls is entered
			DIR *value; 
			struct dirent *direct;
			value = opendir("."); //open the current directory
			
			if (value){
				while ((direct = readdir(value)) != NULL){ //loop over directory and print items
					printf("%s  ", direct->d_name);
				}
				printf("\n");
				closedir(value); //close directory
			}
			continue;
		}

		send(sockfd, message, sizeof(message), 0); //any other message, send to socket

		memset(message, 0, sizeof(message)); //reset message buffer
		recv(sockfd, message, sizeof(message), 0); //recieve message

		if ((strncmp(message, "QUIT", 4)) == 0) { //if server sends quit, disconnect and break
			printf("SERVER DISCONNECTED\n"); 
			break; 
		}

		printf("SERVER: %s", message); //otherwise print server message
	} 

	close(sockfd); //once out of loop, close socket and return 0
	return 0;
} 