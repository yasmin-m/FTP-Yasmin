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
#include <netinet/in.h> 
#include <errno.h>  
#include <arpa/inet.h> 
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#define MAX 1024 //max buffer sizes

int setNewConnection(int portNum, struct sockaddr_in serverAddress, char *IP_ADDRESS){
	int status;
	int mysocket = socket(AF_INET , SOCK_STREAM , 0); //create a socket

	if(mysocket == 0){ //print error message if failed
		printf("ERROR: SOCKET CREATION FAILED\n"); 
		return -1; 
	}

	serverAddress.sin_family = AF_INET; //server connection setup
	serverAddress.sin_addr.s_addr = inet_addr(IP_ADDRESS); 
	serverAddress.sin_port = htons(portNum);

	status = connect(mysocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
	if (status != 0) { //if connection fails, print error
		printf("ERROR: CONNECTION FAILED\n"); 
		return -1; 
	}

	printf("NEW CONNECTION ESTABLISHED TO SERVER\n"); //connection made

	return (mysocket);
}

int main(int argc, char *argv[]) 
{ 
	if (argc != 3){ //if less or more than 3 arguments entered
		printf("USAGE: ./FTPclient ftpserver-ip-address ftpserver-port-number\n");
		exit(0);
	}

	pid_t childpid;
	int sockfd, secondSocket; //socket number
	int status; //temporarily stores status messages
	struct sockaddr_in serverAddress; //server address structure
	char message[MAX]; //buffer stores messages o be sent to server
	int PORT = atoi(argv[2]); //set port number to third argument
	FILE *myfile;
	
	if (PORT == 0){ //ATOI returns zero if it fails
		printf("USAGE: ./FTPclient ftpserver-ip-address ftpserver-port-number\n");
		exit(0);
	}

	sockfd = setNewConnection(PORT, serverAddress, argv[1]);
	if (sockfd == -1){
		return 0;
	}
	 
	for (;;) { //loop over this block, waiting for commands
		printf("ftp> "); //print ftp> at beginning to indicate new command
		memset(message, 0, sizeof(message)); //clear buffer that holds user input
		fgets(message, MAX, stdin); //get user command

		if ((strncmp(message, "QUIT", 4)) == 0) { //if the user command is quit, quit
			printf("CLIENT DISCONNECTED\n"); 
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
				printf("COMMAND FAILED: NO SUCH FILE OR DIRECTORY\n");
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

		if((strncmp(message, "GET ", 4)) == 0){
			send(sockfd, message, sizeof(message), 0);
			
			char filename[MAX];
			memset(filename, 0, MAX);

			int c = 0;
			while (c < strlen(message)-5){   //iterate over the message to get the directory
				filename[c] = message[c+4];
				c++;
			}

			memset(message, 0, sizeof(message));
			recv(sockfd, message, sizeof(message), 0);

			if (strlen(message) > 10){
				printf("SERVER: %s", message);
				continue;
			}

			if((childpid = fork()) == 0){
				int PORT = atoi(message);
				ssize_t len;
				secondSocket = setNewConnection(PORT, serverAddress, argv[1]);

				memset(message, 0, sizeof(message));
				
				recv(secondSocket, message, sizeof(message), 0);
				int filesize = atoi(message);

				myfile = fopen(filename, "w");
				if (myfile == NULL){
					printf("FAILED TO OPEN FILE\n");
					close(secondSocket);
					exit(0);
				}
				
				int remaining = filesize;
				printf("FILE TRANSFER BEGINS\n");
				
				while ((remaining > 0) && (len = recv(secondSocket, message, sizeof(message), 0)) > 0){
					fwrite(message, sizeof(char), len, myfile);
					remaining -= len;
				}

				fclose(myfile);
				printf("FILE TRANSFER SUCCESSFUL\n");
				close(secondSocket);
				return 0;
			}
			continue;
		}

		if((strncmp(message, "PUT ", 4)) == 0){
			char filename[MAX];
			memset(filename, 0, MAX);

			int c = 0;
			while (c < strlen(message)-5){
				filename[c] = message[c+4];
				c++;
			}

			int fileDes = open(filename, O_RDONLY);
			if (fileDes == -1){
				printf("ERROR: FILE DOES NOT EXIST\n");
				continue;
			}

			send(sockfd, message, sizeof(message), 0);
			char message2[MAX];
			memset(message2, 0, sizeof(message2));
			recv(sockfd, message2, sizeof(message2), 0);

			if (strlen(message2) > 10){
				printf("SERVER: %s", message2);
				continue;
			}

			if((childpid = fork()) == 0){
				struct stat st;
				int PORT = atoi(message2);
				ssize_t len;
				secondSocket = setNewConnection(PORT, serverAddress, argv[1]);
				if (secondSocket == -1){
					send(sockfd, "QUIT", 4, 0);
					exit(0);
				}
				
				if (fstat(fileDes, &st) < 0){
					printf("ERROR AQUIRING FILE SIZE\n");
					close(secondSocket);
					return 0;
	        	}

	        	char filesize[MAX];
				sprintf(filesize, "%ld", st.st_size);
				send(secondSocket, filesize, sizeof(filesize), 0);

				off_t offset = 0;
				int remaining = st.st_size;
				int bytesSent;
				printf("FILE TRANSFER BEGIN\n");
				while((bytesSent = sendfile(secondSocket, fileDes, &offset, MAX) > 0) && (remaining > 0)){
					remaining -= bytesSent;
				}
				printf("FILE TRANSFER SUCCESSFUL\n");
				close(fileDes);
				close(secondSocket);
				return 0;
			}
			close(fileDes);
			continue;
		}

		send(sockfd, message, sizeof(message), 0); //any other message, send to socket

		memset(message, 0, sizeof(message)); //reset message buffer
		recv(sockfd, message, sizeof(message), 0); //recieve message

		if ((strncmp(message, "QUIT1", 5)) == 0) {
			printf("MAXIMUM CLIENTS REACHED, SERVER UNABLE TO RESPOND\n");
			break;
		}

		if ((strncmp(message, "QUIT", 4)) == 0) { //if server sends quit, disconnect and break
			printf("SERVER DISCONNECTED\n"); 
			break; 
		}
		
		printf("SERVER: %s", message); //otherwise print server message
	} 

	close(sockfd); //once out of loop, close socket and return 0
	return 0;
} 