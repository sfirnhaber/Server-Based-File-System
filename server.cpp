#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "FileSys.h"

#include <unistd.h>
#include <string.h>
using namespace std;

int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "Usage: ./nfsserver port#\n";
        return -1;
    }
    int port = atoi(argv[1]);

    //networking part: create the socket and accept the client connection

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error: Socket could not be created in server");
        return -1;
    }
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(sock, (struct sockaddr*)&address, sizeof(address)) < 0) { 
        perror("Error: Server could not bind"); 
        return -1;
    }
    if (listen(sock, 1) < 0) {
        perror("Error: Server could not listen");
        return -1;
    }
    int addressSize = sizeof(address);
    int newSock = accept(sock, (struct sockaddr*)&address, (socklen_t*)&addressSize);
    if (newSock < 0) {
        perror("Error: Server could not accept");
        return -1;
    }

    //mount the file system
    FileSys fs;
    fs.mount(newSock); //assume that sock is the new socket created 
                    //for a TCP connection between the client and the server. 
 
    //loop: get the command from the client and invoke the file
    //system operation which returns the results or error messages back to the clinet
    //until the client closes the TCP connection.
    
    
    while (true) {
        
        char message[1024] = {0};
        read(newSock, message, 1024);

        //Parse commands
        char* token = strtok(message, " ");
        char* command;
        char* file_name;
		char* edata;
        command = token;
        token = strtok(NULL, " ");
        file_name = token;
		token = strtok(NULL, " ");
		edata = token;
        
        if (strcmp(command, "") == 0) {
            //return false;
        } else if (strcmp(command, "mkdir") == 0) {
            fs.mkdir(file_name);
        } else if (strcmp(command, "cd") == 0) {
            fs.cd(file_name);
        } else if (strcmp(command, "home") == 0) {
            fs.home();
        } else if (strcmp(command, "rmdir") == 0) {
            fs.rmdir(file_name);
        } else if (strcmp(command, "ls") == 0) {
            fs.ls();
        } else if (strcmp(command, "create") == 0) {
            fs.create(file_name);
        } else if (strcmp(command, "append") == 0) {
            fs.append(file_name, edata);
        } else if (strcmp(command, "cat") == 0) {
            fs.cat(file_name);
        } else if (strcmp(command, "head") == 0) {
            unsigned long n = strtoul(edata, NULL, 0);
            if (edata != NULL) {
              fs.head(file_name, n);
            } else {
              cerr << "Invalid command line: " << command;
              cerr << " is not a valid number of bytes" << endl;
              return false;
            }
        } else if (strcmp(command, "rm") == 0) {
            fs.rm(file_name);
        } else if (strcmp(command, "stat") == 0) {
            fs.stat(file_name);
        } else if (strcmp(command, "quit") == 0) {
            break;
        }
    }
    
    //close the listening socket
    if (close(sock) < 0) {
        perror("Error: Server could not close listening sock");
        return -1;
    }
        
    //unmout the file system
    fs.unmount();

    return 0;
}
