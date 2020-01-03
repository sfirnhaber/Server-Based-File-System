// CPSC 3500: Shell
// Implements a basic shell (command line interface) for the file system

#include <iostream>
#include <fstream>
#include <sstream>

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
using namespace std;

#include "Shell.h"

static const string PROMPT_STRING = "NFS> ";	// shell prompt

// Mount the network file system with server name and port number in the format of server:port
void Shell::mountNFS(string fs_loc) {
	//create the socket cs_sock and connect it to the server and port specified in fs_loc
	//if all the above operations are completed successfully, set is_mounted to true  
    
    is_mounted = true;
    struct sockaddr_in serv_addr;
    
    cs_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (cs_sock < 0)
        printf("Error: Socket could not be created");
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(stoi(fs_loc));
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) { 
        printf("\nInvalid address/ Address not supported \n"); 
        is_mounted = false;
    } 
    if (connect(cs_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error: Connection could not be established");
        is_mounted = false;
    }
    
}

// Unmount the network file system if it was mounted
void Shell::unmountNFS() {
	// close the socket if it was mounted
    if (is_mounted) {
        if (close(cs_sock) < 0)
            perror("Error: Socket could not close");
    }
}

// Remote procedure call on mkdir
void Shell::mkdir_rpc(string dname) {
    char buffer[1024] = {0};
    char message[dname.length() + 6];
    strcpy(message, ("mkdir " + dname).c_str());
    send(cs_sock, message, strlen(message), 0);
    read(cs_sock, buffer, 1024);
    printf("%s\n", buffer);
}

// Remote procedure call on cd
void Shell::cd_rpc(string dname) {
  // to implement
    char buffer[1024] = {0};
    char message[dname.length() + 6];
    strcpy(message, ("cd " + dname).c_str());
    send(cs_sock, message, strlen(message), 0);
    read(cs_sock, buffer, 1024);
    printf("%s\n", buffer);
}

// Remote procedure call on home
void Shell::home_rpc() {
  // to implement
    char buffer[1024] = {0}; 
    const char *message = "home";
    send(cs_sock, message, strlen(message), 0);
    read(cs_sock, buffer, 1024);
    printf("%s\n", buffer);
}

// Remote procedure call on rmdir
void Shell::rmdir_rpc(string dname) {
  // to implement
    char buffer[1024] = {0};
    char message[dname.length() + 6];
    strcpy(message, ("rmdir " + dname).c_str());
    send(cs_sock, &message, strlen(message), 0);
    read(cs_sock, buffer, 1024);
    printf("%s\n", buffer);
}

// Remote procedure call on ls
void Shell::ls_rpc() {
  // to implement
    char buffer[1024] = {0}; 
    const char *message = "ls";
    send(cs_sock, message, strlen(message), 0);
    read(cs_sock, buffer, 1024);
    printf("%s\n", buffer);
}

// Remote procedure call on create
void Shell::create_rpc(string fname) {
  // to implement
    char buffer[1024] = {0};
    char message[fname.length() + 7];
    strcpy(message, ("create " + fname).c_str());
    send(cs_sock, &message, strlen(message), 0);
    read(cs_sock, buffer, 1024);
    printf("%s\n", buffer);
}

// Remote procedure call on append
void Shell::append_rpc(string fname, string data) {
  // to implement
	char buffer[1024] = {0};
    char message[fname.length() + data.length() + 8];
    strcpy(message, ("append " + fname + " " + data).c_str());
    send(cs_sock, &message, strlen(message), 0);
    read(cs_sock, buffer, 1024);
    printf("%s\n", buffer);
}

// Remote procesure call on cat
void Shell::cat_rpc(string fname) {
  // to implement
    char buffer[7680] = {0};
    char message[fname.length() + 4];
    strcpy(message, ("cat " + fname).c_str());
    send(cs_sock, &message, strlen(message), 0);
    read(cs_sock, buffer, 7680);
    printf("%s\n", buffer);
}

// Remote procedure call on head
void Shell::head_rpc(string fname, int n) {
  // to implement
    char buffer[7680] = {0};
    char message[fname.length() + to_string(n).length() + 6];
    strcpy(message, ("head " + fname + " " + to_string(n)).c_str());
    send(cs_sock, &message, strlen(message), 0);
    read(cs_sock, buffer, 7680);
    printf("%s\n", buffer);
}

// Remote procedure call on rm
void Shell::rm_rpc(string fname) {
  // to implement
    char buffer[1024] = {0};
    char message[fname.length() + 3];
    strcpy(message, ("rm " + fname).c_str());
    send(cs_sock, &message, strlen(message), 0);
    read(cs_sock, buffer, 1024);
    printf("%s\n", buffer);
}

// Remote procedure call on stat
void Shell::stat_rpc(string fname) {
  // to implement
    char buffer[1024] = {0};
    char message[fname.length() + 5];
    strcpy(message, ("stat " + fname).c_str());
    send(cs_sock, &message, strlen(message), 0);
    read(cs_sock, buffer, 1024);
    printf("%s\n", buffer);
}

// Executes the shell until the user quits.
void Shell::run()
{
  // make sure that the file system is mounted
  if (!is_mounted)
 	return; 
  
  // continue until the user quits
  bool user_quit = false;
  while (!user_quit) {

    // print prompt and get command line
    string command_str;
    cout << PROMPT_STRING;
    getline(cin, command_str);

    // execute the command
    user_quit = execute_command(command_str);
  }

  // unmount the file system
  unmountNFS();
}

// Execute a script.
void Shell::run_script(char *file_name)
{
  // make sure that the file system is mounted
  if (!is_mounted)
  	return;
  // open script file
  ifstream infile;
  infile.open(file_name);
  if (infile.fail()) {
    cerr << "Could not open script file" << endl;
    return;
  }


  // execute each line in the script
  bool user_quit = false;
  string command_str;
  getline(infile, command_str, '\n');
  while (!infile.eof() && !user_quit) {
    cout << PROMPT_STRING << command_str << endl;
    user_quit = execute_command(command_str);
    getline(infile, command_str);
  }

  // clean up
  unmountNFS();
  infile.close();
}


// Executes the command. Returns true for quit and false otherwise.
bool Shell::execute_command(string command_str)
{
  // parse the command line
  struct Command command = parse_command(command_str);

  // look for the matching command
  if (command.name == "") {
    return false;
  }
  else if (command.name == "mkdir") {
    mkdir_rpc(command.file_name);
  }
  else if (command.name == "cd") {
    cd_rpc(command.file_name);
  }
  else if (command.name == "home") {
    home_rpc();
  }
  else if (command.name == "rmdir") {
    rmdir_rpc(command.file_name);
  }
  else if (command.name == "ls") {
    ls_rpc();
  }
  else if (command.name == "create") {
    create_rpc(command.file_name);
  }
  else if (command.name == "append") {
    append_rpc(command.file_name, command.append_data);
  }
  else if (command.name == "cat") {
    cat_rpc(command.file_name);
  }
  else if (command.name == "head") {
    errno = 0;
    unsigned long n = strtoul(command.append_data.c_str(), NULL, 0);
    if (0 == errno) {
      head_rpc(command.file_name, n);
    } else {
      cerr << "Invalid command line: " << command.append_data;
      cerr << " is not a valid number of bytes" << endl;
      return false;
    }
  }
  else if (command.name == "rm") {
    rm_rpc(command.file_name);
  }
  else if (command.name == "stat") {
    stat_rpc(command.file_name);
  }
  else if (command.name == "quit") {
    const char *message = "quit";
    send(cs_sock, message, strlen(message), 0);
    return true;
  }

  return false;
}

// Parses a command line into a command struct. Returned name is blank
// for invalid command lines.
Shell::Command Shell::parse_command(string command_str)
{
  // empty command struct returned for errors
  struct Command empty = {"", "", ""};

  // grab each of the tokens (if they exist)
  struct Command command;
  istringstream ss(command_str);
  int num_tokens = 0;
  if (ss >> command.name) {
    num_tokens++;
    if (ss >> command.file_name) {
      num_tokens++;
      if (ss >> command.append_data) {
        num_tokens++;
        string junk;
        if (ss >> junk) {
          num_tokens++;
        }
      }
    }
  }

  // Check for empty command line
  if (num_tokens == 0) {
    return empty;
  }
    
  // Check for invalid command lines
  if (command.name == "ls" ||
      command.name == "home" ||
      command.name == "quit")
  {
    if (num_tokens != 1) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else if (command.name == "mkdir" ||
      command.name == "cd"    ||
      command.name == "rmdir" ||
      command.name == "create"||
      command.name == "cat"   ||
      command.name == "rm"    ||
      command.name == "stat")
  {
    if (num_tokens != 2) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else if (command.name == "append" || command.name == "head")
  {
    if (num_tokens != 3) {
      cerr << "Invalid command line: " << command.name;
      cerr << " has improper number of arguments" << endl;
      return empty;
    }
  }
  else {
    cerr << "Invalid command line: " << command.name;
    cerr << " is not a command" << endl; 
    return empty;
  } 

  return command;
}

