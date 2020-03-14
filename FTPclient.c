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

int setNewConnection(int portNum, struct sockaddr_in serverAddress, char *IP_ADDRESS){ //function sets up new connection to server
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

	pid_t childpid; //Child pid
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

	sockfd = setNewConnection(PORT, serverAddress, argv[1]); //set up new connection
	if (sockfd == -1){ //if error occurs, return
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

		if((strncmp(message, "GET ", 4)) == 0){ //if get is entered
			send(sockfd, message, sizeof(message), 0); //send the message over
			
			char filename[MAX]; //stores filename
			memset(filename, 0, MAX); //reset

			int c = 0;
			while (c < strlen(message)-5){   //iterate over the message to get the directory
				filename[c] = message[c+4];
				c++;
			}

			int fileDes = open(filename, O_RDONLY); //open the file based on filename recieved
			if (fileDes != -1){
				printf("FILE NAME EXISTS, PLEASE RENAME FILE AND TRY AGAIN\n");
				continue;
			}

			memset(message, 0, sizeof(message)); //reset message buffer
			recv(sockfd, message, sizeof(message), 0); //recieve either port number or error from server

			if (strlen(message) > 10){ //if it's an error, print if and go to next loop
				printf("SERVER: %s", message);
				continue;
			}

			if((childpid = fork()) == 0){ //fork to set up new tcp connection
				int PORT = atoi(message); //Get port number
				ssize_t len;
				secondSocket = setNewConnection(PORT, serverAddress, argv[1]); //set up connection to server

				memset(message, 0, sizeof(message)); //reset message buffer
				
				recv(secondSocket, message, sizeof(message), 0); //recieve file size
				int filesize = atoi(message); //store as int

				myfile = fopen(filename, "w"); //open the file to write
				if (myfile == NULL){ //if you file to open file, end file transfer
					printf("FAILED TO OPEN FILE\n");
					close(secondSocket);
					exit(0);
				}
				
				int remaining = filesize; //remaining bytes in file
				
				while ((remaining > 0) && (len = recv(secondSocket, message, sizeof(message), 0)) > 0){ //while not all bytes are sent
					fwrite(message, sizeof(char), len, myfile);
					remaining -= len;
				}

				fclose(myfile); //close file
				printf("FILE TRANSFER SUCCESSFUL\n");
				printf("ftp> ");
				close(secondSocket); //close second tcp connection
				return 0; //end child process
			}
			continue; //move onto next loop
		}

		if((strncmp(message, "PUT ", 4)) == 0){ //if put is entered
			char filename[MAX]; //stores file name
			memset(filename, 0, MAX); //reset data stored in here

			int c = 0;
			while (c < strlen(message)-5){
				filename[c] = message[c+4];
				c++;
			} //get filename

			int fileDes = open(filename, O_RDONLY);
			if (fileDes == -1){
				printf("ERROR: FILE DOES NOT EXIST\n");
				continue;
			} //open the file and check if it exists

			send(sockfd, message, sizeof(message), 0); //if it exists, send command to server
			char message2[MAX]; //new message buffer
			memset(message2, 0, sizeof(message2)); 
			recv(sockfd, message2, sizeof(message2), 0); //recieve port number or error message

			if (strlen(message2) > 10){ //if greater than 10, then its an error message
				printf("SERVER: %s", message2);
				continue;
			}

			if((childpid = fork()) == 0){ //fork child process
				struct stat st;
				int PORT = atoi(message2);
				ssize_t len;
				secondSocket = setNewConnection(PORT, serverAddress, argv[1]); //set up a new connection to server

				if (fstat(fileDes, &st) < 0){
					printf("ERROR AQUIRING FILE SIZE\n");
					close(secondSocket);
					return 0;
	        	} //if error occured aquiring file size, then end connection

	        	char filesize[MAX]; //stores filesize
				sprintf(filesize, "%ld", st.st_size); //get filesize and convert to string
				send(secondSocket, filesize, sizeof(filesize), 0); //send filesize

				off_t offset = 0; //beginning of file
				int remaining = st.st_size;
				int bytesSent;
				while((bytesSent = sendfile(secondSocket, fileDes, &offset, MAX) > 0) && (remaining > 0)){
					remaining -= bytesSent;
				}
				printf("FILE TRANSFER SUCCESSFUL\n"); //file transfer end
				printf("ftp> ");
				close(fileDes); //close file descriptor
				close(secondSocket); //close second tcp connection
				return 0; //end child process
			}
			close(fileDes); //close file descriptor in parent process
			continue; //move onto next command
		}

		send(sockfd, message, sizeof(message), 0); //any other message, send to socket

		memset(message, 0, sizeof(message)); //reset message buffer
		recv(sockfd, message, sizeof(message), 0); //recieve message

		if ((strncmp(message, "QUIT1", 5)) == 0) { //if quit1 recieved, then max number of clients reached
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