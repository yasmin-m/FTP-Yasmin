all: FTPserver FTPclient

TCPclient: FTPclient.c
	gcc FTPclient.c -o FTPclient

TCPserver: FTPserver.c
	gcc FTPserver.c -o FTPserver

clean:
	-rm -f FTPclient FTPserver