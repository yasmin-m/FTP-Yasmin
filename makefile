all: FTPserver FTPclient

TCPclient: FTPclient.c
	gcc -Wall -Wextra -g FTPclient.c -o FTPclient

TCPserver: FTPserver.c
	gcc -Wall -Wextra -g FTPserver.c -o FTPserver

clean:
	-rm -f FTPclient FTPserver