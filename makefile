all: FTPserver FTPclient

FTPclient: FTPclient.c
	gcc -Wall -Wextra -g FTPclient.c -o FTPclient

FTPserver: FTPserver.c
	gcc -Wall -Wextra -g FTPserver.c -o FTPserver

clean:
	-rm -f FTPclient FTPserver
