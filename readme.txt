# FTP Server and Client

FTP Server and Client are two programmes for handling the file transfer between 
an FTP client and an FTP server.

## Installation

To install, invoke make in a shell with gcc compiler
To uninstall, invoke make clean in a shell with gcc compiler

## Usage

To run the FTP server, invoke ./FTPserver
To run the FTP client, invoke ./FTPclient ip_address port_num where ip_address
and port_num refer to the IP address and port number of the FTP server respectively.

By default, FTP server has IP address 127.0.0.1 and Port Number 8000

A username and password are required for the client to use the FTPserver. This
information is stored in userinfo.txt. Commands that do not require the server
(ex. client-side file navigation) do not require the user to be logged in.

## Commands

 - USER username   - Use to enter username
 - PASS password   - Use to enter password
 - GET filename    - Use to get file from server
 - PUT filename    - Use to put file in server
 - LS			   - Use to list files in server directory
 - !LS 			   - Use to list files in client directory
 - PWD			   - Print current working server directory
 - !PWD			   - Print current working client directoty
 - CD directory    - Change current server directory
 - !CD directory   - Change current client directory
 - QUIT			   - Exit

 