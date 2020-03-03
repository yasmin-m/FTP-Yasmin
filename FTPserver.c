#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h>
#include <errno.h>  
#include <unistd.h>  
#include <arpa/inet.h> 
#include <sys/time.h>

#define PORT 8000 //Port number
#define MAX_CLIENTS 10 //Maximum number of clients
#define IP_ADDRESS "127.0.0.1" //IP address
#define USER_AND_PASS "userinfo.txt" //text file with usernames and passwords
#define MAX 1024 //maximum buffer size

int main() 
{
	int socket_list[MAX_CLIENTS]; //list of sockets
	struct sockaddr_in serverAddress; //structure for server address
	int status; //stores status erors of functions
	char messBuff[MAX]; //stores messages sent and recieved between clients
	fd_set fdset; //define fdset for select function
	
	memset(socket_list, 0, MAX_CLIENTS); //set socket list to 0

	int firstSocket = socket(AF_INET , SOCK_STREAM , 0); //create a socket
	if(firstSocket == 0){ //print error message if failed
		printf("ERROR: SOCKET CREATION FAILED\n"); 
		exit(0); 
	}

	serverAddress.sin_family = AF_INET; //server connection setup
	serverAddress.sin_addr.s_addr = inet_addr(IP_ADDRESS); 
	serverAddress.sin_port = htons(PORT);
	
	status = bind(firstSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)); //bind the socket	
	if (status != 0){ //Print error message 
		printf("ERROR: BINDING FAILED\n"); 
		exit(0); 
	}

	printf("SERVER READY\n"); //server bound and preparing to listen

	status = listen(firstSocket, 5); //listen for up to 5 at a time
	if (status != 0){ //print error message
		printf("ERROR: LISTENING FAILED\n"); 
		exit(0); 
	}
	
	int authenticate_list[MAX_CLIENTS]; //list of authenticated users
	memset(authenticate_list, 0, MAX_CLIENTS); //empty the list of authenticated users
	char passwords[MAX_CLIENTS][MAX]; //list of passwords for authenticated users
	for (int i=0; i<MAX_CLIENTS; i++){ //empty list of passwords
		memset(passwords[i], 0, MAX);
	}

	for(;;){ //while connection is on
		FD_ZERO(&fdset); //zero everything
		FD_SET(firstSocket, &fdset); //set based on the clients
		int maxSockDes = firstSocket; //maximum socket descriptor
		int sockDes; //socket descriptor
			
		for (int i=0; i<MAX_CLIENTS; i++){ //update maximum socket descriptor
			sockDes = socket_list[i];

			if(sockDes > 0){ 
				FD_SET(sockDes, &fdset); 
			}

			if(sockDes > maxSockDes){
				maxSockDes = sockDes; 
			}
		} 

		int activity = select(maxSockDes+1, &fdset, NULL, NULL, NULL); //select max socket descriptor
		int len = sizeof(serverAddress); //define lne
	
		status = FD_ISSET(firstSocket, &fdset); //if someone is listening, accept connection
		if (status) 
		{
			int clientSocket = accept(firstSocket, (struct sockaddr*)&serverAddress, (socklen_t*)&len); //accpet client
			if (clientSocket < 0){ //print error
				printf("ERROR: ACCEPT FAILED\n"); 
				exit(0); 
			}  
			
			printf("NEW CLIENT CONNECTED\n"); //print message

			for (int i=0; i<MAX_CLIENTS; i++) { //add client to client list
				if(socket_list[i] == 0) { 
					socket_list[i] = clientSocket; 
					break; 
				}
			} 
		} 
 
		for (int i=0; i<MAX_CLIENTS; i++) //loop over all the clients
		{ 
			sockDes = socket_list[i]; //socket descriptor
			char *sendThis; //message to be sent at the end of everything
			int responded = 0; //Flag, if set to one continue rest of loop
				
			if (FD_ISSET(sockDes ,&fdset)){ //if they have something to say
				int message = read(sockDes, messBuff, MAX); //read the incoming message
				
				if (message == 0){ //if the message is 0, the client has disconnected
					printf("CLIENT DISCONNECTED\n");
					authenticate_list[i]=0; //remove them from the authenticated user list
					memset(passwords[i], 0, MAX); //remove their password
					close(sockDes); //close their socket
					socket_list[i] = 0; //remove them from the socket list
					responded = 1; //set responded to 1
				}

				// if they typed USER and a response hasn't already been sent
				if(responded == 0 && (strncmp(messBuff, "USER ", 5)) == 0 && strlen(messBuff)>7){
					//if they haven't been authenticated
					if (authenticate_list[i]==0){
						char myuser[MAX];
						memset(myuser, 0, sizeof(myuser)); //stores their username
						for(int j=0; j<strlen(messBuff)-6; j++){ //check there are no empty spaces
							if(messBuff[j+5] == ' '){
								sendThis = "NO EMPTY SPACES IN USERNAME\n";
								send(sockDes, sendThis, strlen(sendThis), 0);
								responded = 1; //if there are respond and break
								break;
							}
							myuser[j] = messBuff[j+5]; //stores username
						}

						FILE *userpass = fopen(USER_AND_PASS, "r"); //open the list of usernames and passwords
						if (userpass == NULL){
							printf("USER NAME AND PASSWORD INFORMATION COULD NOT BE FOUND\n");
							exit(0);
						}

						char str[MAX];
						if(!responded){ //if no response has been issued
							while (fgets(str, sizeof(str), userpass)){ //compare username to file
								if (strncmp(myuser, str, strlen(myuser)) == 0){ //if found
									
									sendThis = "USERNAME OK, PASSWORD REQUIRED\n";
									send(sockDes, sendThis, strlen(sendThis), 0); 
									authenticate_list[i]=1;
									responded = 1; //respond with 1
									
									for(int j=0; j<strlen(str)-strlen(messBuff)+4; j++){
										passwords[i][j] = str[j+strlen(messBuff)-5]; //store password so you don't have to reretrieve it
									}
									break;
								}
							}
							fclose(userpass); //close file
						}
						
						if(!responded){ //if no response was issued, it's becaue it wasn't recognized
							sendThis = "USERNAME NOT RECOGNIZED\n";
							send(sockDes, sendThis, strlen(sendThis), 0);
							responded = 1;
						}
						
					}
					else{ //if flag is 1 or 2, username already entered
						sendThis = "USERNAME ALREADY ENTERED\n";
						send(sockDes, sendThis, strlen(sendThis), 0);
						responded = 1;
					}
				}

				if ((strncmp(messBuff, "PASS ", 5)) == 0 && strlen(messBuff)>7){ //if pass is entered
					if (responded == 0 && authenticate_list[i]==1){
						char mypass[MAX];
						memset(mypass, 0, sizeof(mypass));
						for(int j=0; j<strlen(messBuff)-6; j++){ //check there are no blank spaces
							if(messBuff[j+5] == ' '){
								sendThis = "NO EMPTY SPACES IN PASSWORD\n";
								send(sockDes, sendThis, strlen(sendThis), 0);
								responded = 1;
								break;
							}
							mypass[j] = messBuff[j+5]; //entered password
						}

						if (strncmp(mypass, passwords[i], strlen(mypass)) == 0){ //password same as stored in array
							sendThis = "LOGIN SUCCESSFUL\n";
							send(sockDes, sendThis, strlen(sendThis), 0);
							responded = 1;
							authenticate_list[i]=2;
						}

						if(!responded){ //password was not recognized
							sendThis = "PASSWORD NOT RECOGNIZED\n";
							send(sockDes, sendThis, strlen(sendThis), 0);
							responded = 1;
						}
					}
					if (responded == 0 && authenticate_list[i]==2){ //if already logged in
						sendThis = "USER ALREADY LOGGED IN\n";
						send(sockDes, sendThis, strlen(sendThis), 0);
						responded = 1;
					}
				}

				if(responded == 0 && authenticate_list[i]==0){ //if anything else is entered before authentication
					sendThis = "ENTER USERNAME FIRST\n";
					send(sockDes, sendThis, strlen(sendThis), 0); 
					responded = 1;
				}

				if(responded == 0 && authenticate_list[i]==1){ //if anything else is entered before authentication
					sendThis = "PASSWORD AUTHENTICATION PENDING\n";
					send(sockDes, sendThis, strlen(sendThis), 0);
					responded = 1;
				}

				if(responded == 0){ //unrecognized command
					sendThis = "COMMAND NOT RECOGNIZED\n";
					send(sockDes, sendThis, strlen(sendThis), 0);
				}
			} 
		} 
	} 
		
	return 0; 
} 
