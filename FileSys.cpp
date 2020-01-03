// CPSC 3500: File System
// Implements the file system commands that are available to the shell.

#include <cstring>
#include <iostream>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

#include "FileSys.h"
#include "BasicFileSys.h"
#include "Blocks.h"

// mounts the file system
void FileSys::mount(int sock) {
  bfs.mount();
  curr_dir = 1; //by default current directory is home directory, in disk block #1
  fs_sock = sock; //use this socket to receive file system operations from the client and send back response messages
}

// unmounts the file system
void FileSys::unmount() {
  bfs.unmount();
  close(fs_sock);
}

// make a directory
void FileSys::mkdir(const char *name) {
  
    short blockNum = bfs.get_free_block();
    if (blockNum == 0) {
        const char *failure = "505 Disk is full";
        send(fs_sock, failure, strlen(failure), 0);
        return;
    }
    if (strlen(name) > MAX_FNAME_SIZE) {
        const char *failure = "504 File name is too long";
        send(fs_sock, failure, strlen(failure), 0);
        return;
    }
    
    dirblock_t current;
    bfs.read_block(curr_dir, &current);
    
    //Checks if file of same name already exists
    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (strcmp(current.dir_entries[i].name, name) == 0) {
            const char *failure = "502 File exists";
            send(fs_sock, failure, strlen(failure), 0);
            return;
        }
    }
	
	//Updates the current directory to accommodate new file
    int i;
    for (i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (current.dir_entries[i].block_num == 0) {
            current.dir_entries[i].block_num = blockNum;
            strncpy(current.dir_entries[i].name, name, sizeof(current.dir_entries[i].name));
            break;
        }
    }
    
    //Fails if directory is full. Otherwise increment parent directory and initialize new directory values
    if (i == MAX_DIR_ENTRIES) {
        const char *failure = "506 Directory is full";
        send(fs_sock, failure, strlen(failure), 0);
    } else {
        current.num_entries++;
        bfs.write_block(curr_dir, &current);
        dirblock_t dir;
        dir.magic = DIR_MAGIC_NUM;
        dir.num_entries = 0;
        for (int j = 0; j < MAX_DIR_ENTRIES; j++)
            dir.dir_entries[j].block_num = 0;
        bfs.write_block(blockNum, &dir);
        const char *success = "success";
        send(fs_sock, success, strlen(success), 0);
    }
    
}

// switch to a directory
void FileSys::cd(const char *name) {
	dirblock_t current;
	bfs.read_block(curr_dir, &current); //Get directory data of current directory
	
	int i;
	for(i = 0; i < MAX_DIR_ENTRIES; i++) {
	
        dirblock_t destination;
        bfs.read_block(current.dir_entries[i].block_num, &destination);
		//Check for the name of directory to be present and that it's a directory and not a file
		if(strcmp(current.dir_entries[i].name, name) == 0 && destination.magic == DIR_MAGIC_NUM) {
			break;
		} else if (strcmp(current.dir_entries[i].name, name) == 0 && destination.magic != DIR_MAGIC_NUM) {
            const char* failure = "500 File is not a directory";
            send(fs_sock, failure, strlen(failure), 0);
            return;
        }
	}
	
	if(i == MAX_DIR_ENTRIES) { //Sends fail message if target dir is not found
		const char* failure = "503 File does not exist";
		send(fs_sock, failure, strlen(failure), 0);
	} else {
		curr_dir = current.dir_entries[i].block_num; //Sets current directory to new directory
        const char* success = "success";
		send(fs_sock, success, strlen(success), 0);
	}
}

// switch to home directory
void FileSys::home() {
	curr_dir = 1;
    const char* success = "success";
	send(fs_sock, success, strlen(success), 0);
}

// remove a directory
void FileSys::rmdir(const char *name) {
	dirblock_t current;
	bfs.read_block(curr_dir, &current);
    dirblock_t destination;
	
	int i;
	for(i = 0; i < MAX_DIR_ENTRIES; i++) {

        bfs.read_block(current.dir_entries[i].block_num, &destination);
		//Check for the name of directory to be present and that it's a directory and not a file
		if(strcmp(current.dir_entries[i].name, name) == 0 && destination.magic == DIR_MAGIC_NUM) {
			break;
		} else if (strcmp(current.dir_entries[i].name, name) == 0 && destination.magic != DIR_MAGIC_NUM) {
            const char* failure = "500 File is not a directory";
            send(fs_sock, failure, strlen(failure), 0);
            return;
        }
	}
	
	//Sends fail message if target dir is not found
	if(i == MAX_DIR_ENTRIES) {
		const char* failure = "503 File does not exist";
		send(fs_sock, failure, strlen(failure), 0);
	} else {
		//Checks if the target directory is empty. Cannot remove if it's not empty.
		if(destination.num_entries == 0) {
			bfs.reclaim_block(current.dir_entries[i].block_num); //Free block used by the dir being remove
            current.dir_entries[i].block_num = 0; //Set current dir back to empty entry
			current.num_entries--; //Decrement the entries in the current directory
            strncpy(current.dir_entries[i].name, "", sizeof(current.dir_entries[i].name));
            bfs.write_block(curr_dir, &current);
			
			const char* success = "success";
			send(fs_sock, success, strlen(success), 0);
		} else {
			const char* full = "507 Directory is not empty";
			send(fs_sock, full, strlen(full), 0);
		}
				
	}
	
}

// list the contents of current directory
void FileSys::ls() {
	dirblock_t current;
	bfs.read_block(curr_dir, &current); //Gets the data in the block of current directory
	
	char output[1024] = {0};
    bool empty = true;
	for(int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (current.dir_entries[i].block_num != 0) {
            strcat(output, current.dir_entries[i].name);
            dirblock_t temp;
            bfs.read_block(current.dir_entries[i].block_num, &temp);
            if (temp.magic == DIR_MAGIC_NUM)
                strcat(output, "/");
            strcat(output, " ");
            empty = false;
        }
    }
    if (empty) {
        const char* failure = "empty folder";
        send(fs_sock, failure, strlen(failure), 0);
        return;
    }
    send(fs_sock, output, strlen(output), 0);
}

// create an empty data file
void FileSys::create(const char *name) {
    
    short blockNum;
    dirblock_t current;
	bfs.read_block(curr_dir, &current); //Gets the data in the block of current directory
    if (strlen(name) > MAX_FNAME_SIZE) {
        const char *failure = "504 File name is too long";
        send(fs_sock, failure, strlen(failure), 0);
        return;
    }
    int i;
    for(i = 0; i < MAX_DIR_ENTRIES; i++) {
        inode_t temp;
        bfs.read_block(current.dir_entries[i].block_num, &temp);
        if(strcmp(current.dir_entries[i].name, name) == 0 && temp.magic == INODE_MAGIC_NUM) {
            const char* failure = "502 File already exists";
            send(fs_sock, failure, strlen(failure), 0);
			return;
		}
        if (current.dir_entries[i].block_num == 0) {
            blockNum = bfs.get_free_block();
            if (blockNum == 0) {
                const char *failure = "505 Disk is full";
                send(fs_sock, failure, strlen(failure), 0);
                return;
            }
            current.dir_entries[i].block_num = blockNum;
            strncpy(current.dir_entries[i].name, name, sizeof(current.dir_entries[i].name));
            current.num_entries++;
            break;
        }
    }
    
    if(i == MAX_DIR_ENTRIES) {
		const char* failure = "506 Directory is full";
		send(fs_sock, failure, strlen(failure), 0);
	} else {
        
        inode_t inode;
        inode.magic = INODE_MAGIC_NUM;
        inode.size = 0;
        bfs.write_block(blockNum, &inode);
        bfs.write_block(curr_dir, &current);
        
        const char* success = "success";
		send(fs_sock, success, strlen(success), 0);
    }
    
}

// append data to a data file
void FileSys::append(const char *name, const char *data) {
    
	// First get file from name
	dirblock_t current;
	bfs.read_block(curr_dir, &current); //Gets the data in the block of current directory
	int i; // Gets location of block file in directory
    inode_t target;
	for(i = 0; i < MAX_DIR_ENTRIES; i++) {
        bfs.read_block(current.dir_entries[i].block_num, &target);
        if(strcmp(current.dir_entries[i].name, name) == 0 && target.magic == INODE_MAGIC_NUM) {
			break;
		} else if (strcmp(current.dir_entries[i].name, name) == 0 && target.magic != INODE_MAGIC_NUM) {
            const char* failure = "501 File is a directory";
            send(fs_sock, failure, strlen(failure), 0);
            return;
        }
    }
    if(target.size + strlen(data) > MAX_FILE_SIZE) {
        const char* failure = "508 Append exceeds maximum file size";
		send(fs_sock, failure, strlen(failure), 0);
        return;
    }
    if(i == MAX_DIR_ENTRIES) { //Sends fail message if target dir is not found
		const char* failure = "503 File does not exist";
		send(fs_sock, failure, strlen(failure), 0);
        return;
	}
    
    int size = target.size;
    int blocksUsed = 0;
    int currentIndex;
    for (currentIndex = size; currentIndex > BLOCK_SIZE; currentIndex -= BLOCK_SIZE) {
        blocksUsed++;
    }
    blocksUsed++;
    
    int index = 0;
    if (target.size == 0) {
        short blockNum = bfs.get_free_block();
        if (blockNum == 0) {
            const char* failure = "505 Disk is full";
            send(fs_sock, failure, strlen(failure), 0);
            return;
        }
        target.blocks[0] = blockNum;
    }
    for (int j = blocksUsed; j < MAX_DATA_BLOCKS; j++) {
        datablock_t block;
        bfs.read_block(target.blocks[j - 1], &block);
        for (int k = currentIndex; k < BLOCK_SIZE; k++) {
            block.data[k] = data[index];
            index++;
            if (index == strlen(data))
                break;
        }
        bfs.write_block(target.blocks[j - 1], &block);
        if (index == strlen(data))
            break;
        short blockNum = bfs.get_free_block();
        if (blockNum == 0) {
            const char* failure = "505 Disk is full";
            send(fs_sock, failure, strlen(failure), 0);
            return;
        }
        target.blocks[j] = blockNum;
        currentIndex = 0;
    }
    target.size += strlen(data);
    bfs.write_block(current.dir_entries[i].block_num, &target);
    bfs.write_block(curr_dir, &current);
	
	const char* success = "success";
	send(fs_sock, success, strlen(success), 0);
}

// display the contents of a data file
void FileSys::cat(const char *name) {
	// First get file from name
	dirblock_t current;
	bfs.read_block(curr_dir, &current); //Gets the data in the block of current directory
	int i; // Gets location of block file in directory
    inode_t target;
	for(i = 0; i < MAX_DIR_ENTRIES; i++) {
        bfs.read_block(current.dir_entries[i].block_num, &target);
        if(strcmp(current.dir_entries[i].name, name) == 0 && target.magic == INODE_MAGIC_NUM) {
			break;
		} else if (strcmp(current.dir_entries[i].name, name) == 0 && target.magic != INODE_MAGIC_NUM) {
            const char* failure = "501 File is a directory";
            send(fs_sock, failure, strlen(failure), 0);
            return;
        }
    }
    if (i == MAX_DIR_ENTRIES) {
        const char* failure = "503 File does not exist";
        send(fs_sock, failure, strlen(failure), 0);
    }
    
    int blocksUsed = 0;
    for (int j = target.size; j > 0; j -= BLOCK_SIZE) {
        blocksUsed++;
    }
    
    if (blocksUsed == 0) {
        return;
        //Nothing in file
    }
        
    char output[MAX_FILE_SIZE];
    int index = 0;
    for (int j = 0; j < blocksUsed; j++) {
        datablock_t block;
        bfs.read_block(target.blocks[j], &block);
        for (int k = 0; k < BLOCK_SIZE; k++) {
            output[index] = block.data[k];
            index++;
            if (target.size == index)
                break;
        }
    }
    
    send(fs_sock, output, strlen(output), 0);
    
}

// display the first N bytes of the file
void FileSys::head(const char *name, unsigned int n) {
    
    // First get file from name
	dirblock_t current;
	bfs.read_block(curr_dir, &current); //Gets the data in the block of current directory
	int i; // Gets location of block file in directory
    inode_t target;
	for(i = 0; i < MAX_DIR_ENTRIES; i++) {
        bfs.read_block(current.dir_entries[i].block_num, &target);
        if(strcmp(current.dir_entries[i].name, name) == 0 && target.magic == INODE_MAGIC_NUM) {
			break;
		} else if (strcmp(current.dir_entries[i].name, name) == 0 && target.magic != INODE_MAGIC_NUM) {
            const char* failure = "501 File is a directory";
            send(fs_sock, failure, strlen(failure), 0);
            return;
        }
    }
    if (i == MAX_DIR_ENTRIES) {
        const char* failure = "503 File does not exist";
        send(fs_sock, failure, strlen(failure), 0);
        return;
    }
    if (n > target.size)
        n = target.size;
    
    int blocksUsed = 0;
    for (int j = n; j > 0; j -= BLOCK_SIZE) {
        blocksUsed++;
    }
    
    if (blocksUsed == 0) {
        const char* failure = " ";
        send(fs_sock, failure, strlen(failure), 0);
        return;
        //Nothing in file
    }
    
    char output[MAX_FILE_SIZE];
    for (int i = 0; i < MAX_FILE_SIZE; i++)
        output[i] = '\0';
    int index = 0;
    for (int j = 0; j < blocksUsed; j++) {
        datablock_t block;
        bfs.read_block(target.blocks[j], &block);
        for (int k = 0; k < BLOCK_SIZE; k++) {
            output[index] = block.data[k];
            index++;
            if (n == index)
                break;
        }
        if (n == index)
            break;
    }
    
    send(fs_sock, output, strlen(output), 0);
    
}

// delete a data file
void FileSys::rm(const char *name) {
    dirblock_t current;
	inode_t target;
	bfs.read_block(curr_dir, &current); //Gets the data in the block of current directory
	int i; 
	for(i = 0; i < MAX_DIR_ENTRIES; i++) {
        inode_t temp;
        bfs.read_block(current.dir_entries[i].block_num, &temp);
        if(strcmp(current.dir_entries[i].name, name) == 0 && temp.magic == INODE_MAGIC_NUM) {
			break;
		} else if (strcmp(current.dir_entries[i].name, name) == 0 && temp.magic != INODE_MAGIC_NUM) {
            const char* failure = "501 File is a directory";
            send(fs_sock, failure, strlen(failure), 0);
            return;
        }
    }
    if(i == MAX_DIR_ENTRIES) {
		const char* failure = "503 File does not exist";
		send(fs_sock, failure, strlen(failure), 0);
        return;
	}
	
	bfs.read_block(current.dir_entries[i].block_num, &target);
	// First delete all the data associated with it, then delete the file itself
	
	int numOfBlocks;
	if (target.size == 0)
		numOfBlocks = 0;
	else 
		numOfBlocks = (target.size-1)/BLOCK_SIZE+1;

	for (int j = 0; j < numOfBlocks; j++)
		bfs.reclaim_block(target.blocks[j]);
    
	bfs.reclaim_block(current.dir_entries[i].block_num);
	strncpy(current.dir_entries[i].name, "", sizeof(current.dir_entries[i].name));
	current.dir_entries[i].block_num = 0;
    current.num_entries--;
	bfs.write_block(curr_dir, &current);
		
	const char* success = "success";
	send(fs_sock, success, strlen(success), 0);
}

// display stats about file or directory
void FileSys::stat(const char *name) {
    dirblock_t current;
	bfs.read_block(curr_dir, &current); //Gets the data in the block of current directory
	int i; 
	for(i = 0; i < MAX_DIR_ENTRIES; i++) {
        if(strcmp(current.dir_entries[i].name, name) == 0) {
			break;
		}
    }
    if (i == MAX_DIR_ENTRIES) {
        const char* failure = "503 File does not exist";
        send(fs_sock, failure, strlen(failure), 0);
        return;
    }
    
    inode_t inode;
    bfs.read_block(current.dir_entries[i].block_num, &inode);
    char output[1024] = {0};
    if (inode.magic == INODE_MAGIC_NUM) {
        strcat(output, "Inode block: ");
        int temp2 = current.dir_entries[i].block_num;
        const char* temp = to_string(temp2).c_str();
        strcat(output, temp);
        strcat(output, "\nBytes in file: ");
        const char* temp3 = to_string(inode.size).c_str();
        strcat(output, temp3);
        strcat(output, "\nNumber of blocks: ");
        if (inode.size == 0) {
            const char* temp4 = to_string(1).c_str();
            strcat(output, temp4);
        } else {
            const char* temp4 = to_string((inode.size / BLOCK_SIZE) + 2).c_str();
            strcat(output, temp4);
        }
        strcat(output, "\nFirst Block: ");
        if (inode.size == 0) {
            const char* temp4 = to_string(0).c_str();
            strcat(output, temp4);
        } else {
            const char* temp4 = to_string(inode.blocks[0]).c_str();
            strcat(output, temp4);
        }
        
    } else {
        strcat(output, "Directory name: ");
        strcat(output, current.dir_entries[i].name);
        strcat(output, "/\nDirectory block: ");
        int temp2 = current.dir_entries[i].block_num;
        const char* temp = to_string(temp2).c_str();
        strcat(output, temp);
    } 
    send(fs_sock, output, strlen(output), 0);
    
}

// HELPER FUNCTIONS (optional)

