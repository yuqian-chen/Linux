#include "mfs.h"
#include "udp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct bitmap {
 	int inodes[64];
	int data[1024];
}bitmap;

typedef struct info {
	char name[60];
	int inum;
}info; 

typedef struct infochunk {
	struct info inf[64];
}infochunk; 
	
int port;
int fd;
int fd2;
int i;
struct sockaddr_in client;
char header_blocks[3*BSIZE];
struct superblock *sb = (struct superblock*) &header_blocks[0*BSIZE];
struct dinode *inodes = (struct dinode*) &header_blocks[1*BSIZE];
struct bitmap *bm = (struct bitmap*) &header_blocks[2*BSIZE]; 
message msg;
response res;

int test( int Arr[ ],  int k ){
    unsigned int flag = 1;  
    flag = flag << k%32;     
    if ( Arr[k/32] & flag ) {
    	return 1;
    }     
    else{
    	return 0;
 	}
}
void lookup(){
	if (test(bm->inodes,msg.inum) == 0) {
		res.rc = -1;
	}else{
		struct infochunk ichunk;
		int rc = -1;
		int blocks = (inodes + msg.inum)->addrs[0];
		int p = pread(fd2, &ichunk, 1*BSIZE, (blocks + 3)*BSIZE);
		if (p != -1) {
			for (i = 0; i < 64; i++){	
				if (strcmp(ichunk.inf[i].name, msg.name) == 0 && ichunk.inf[i].inum != -1 ) {
					rc = ichunk.inf[i].inum;
				}
			}
		}
		res.rc = rc;
	}
	UDP_Write(fd, &client, (char*) &res, sizeof(response));
}

void stat(){
	int n = 0;
	MFS_Stat_t metainfo;

	pread(fd2, header_blocks, 3*BSIZE, 0);
	struct dinode* inode = (inodes + msg.inum);

	if (inode->type == MFS_REGULAR_FILE) {
		for (i = 1; i <= NDIRECT + 1; i++) {
			if (inode->addrs[i-1] != 0) {
				n = 4096 * i;
			}
		}
	}
	metainfo.size = n;
	metainfo.type = inode->type;
	res.stat = metainfo;
	res.rc = 0;
	UDP_Write(fd, &client, (char*) &res, sizeof(response));
}
void writes(){
	struct dinode* inode = (inodes + msg.inum);
	int blocks = inode->addrs[msg.blocknum];
	unsigned int flag = 1;   

	if (msg.blocknum > NDIRECT || inode->type != MFS_REGULAR_FILE) {
		res.rc = -1;
	}else{
		int found = 0;
		if (blocks == 0) {
			for (i = 0; i < 1024 && found == 0; i++){
				if (test(bm->data, i) == 0) {
      				flag = flag << i%32;      
      				bm->data[i/32] = bm->data[i/32] | flag;
					blocks = i;
					found = 1;
				}
			}
			inode->addrs[msg.blocknum] = blocks;
		}
		res.rc = 0;
		pwrite(fd2, header_blocks, 3 * BSIZE, 0);
		pwrite(fd2, msg.block, BSIZE, (3 + blocks) * BSIZE);
		fsync(fd2);
	}
	UDP_Write(fd, &client, (char*) &res, sizeof(response));
}
void reads(){
	struct dinode* inode = (inodes + msg.inum);
	int blocks = inode->addrs[msg.blocknum];

	if ( msg.blocknum > NDIRECT || inode->type != MFS_REGULAR_FILE) {
		res.rc = -1;
	}else{
		res.rc = 0;
		pread(fd2, res.block, BSIZE, (3 + blocks) * BSIZE);
		fsync(fd2);
	}
	UDP_Write(fd, &client, (char*) &res, sizeof(response));
}
void create(){
	struct dinode* inode = (inodes + msg.inum);
	int blocks = inode->addrs[0];

	if (inode->type != MFS_DIRECTORY) {
		res.rc = -1;
	}
	else{
		struct infochunk ichunk;
		int p = pread(fd2, &ichunk, BSIZE, (3 + blocks) * BSIZE);
		struct dinode* newI;
		struct infochunk newchunk;		
		int inum;
		int inum2;
		int found = 0;

		for (i = 0; i < 64 && found == 0; i++){
			if (!test(bm->inodes, i)) {
				unsigned int flag = 1;   
      			flag = flag << i % 32;      
      			bm->inodes[i/32] = bm->inodes[i/32] | flag;
				inum = i;
				found =1;
			}
		}
		newI = (inodes + inum);
		newI->size = 0;
		newI->type = msg.type;

		found = 0;
		if (p != -1) {
			for (i = 0; i < 64 && found == 0; i++){
				if (ichunk.inf[i].inum == -1){
					strncpy(ichunk.inf[i].name, msg.name, 60);
					ichunk.inf[i].inum = inum;
					found =1;
				}
			}
		}
		if (newI->type == MFS_DIRECTORY) {
			found = 0;
			for (i = 0; i < 1024 && found == 0; i++){
				if (test(bm->data, i) == 0) {
					unsigned int flag = 1;   
      				flag = flag << i%32;      
      				bm->data[i/32] = bm->data[i/32] | flag;    
					inum2 = i;
					found =1;
				}
			}
			newI->addrs[0] = inum2;
			strncpy(newchunk.inf[0].name, ".", 1);
			newchunk.inf[0].inum = inum;

			strncpy(newchunk.inf[1].name, ".." , 2);
			newchunk.inf[1].inum = msg.inum;

			for (i = 2; i < 64; i++){
				newchunk.inf[i].inum = -1;
			}
			pwrite(fd2, &newchunk, BSIZE, (3 + inum2)*BSIZE);
		}

		sb->size+=1;			
		sb->nblocks+=1;
		sb->ninodes+=1;
		res.rc = 0;
		pwrite(fd2, &ichunk, BSIZE, (3 + blocks) * BSIZE);
		pwrite(fd2, header_blocks, 3*BSIZE, 0);
		fsync(fd2);
	}
	UDP_Write(fd, &client, (char*) &res, sizeof(response));
}

void unlinks(){
	struct infochunk ichunk;
	struct dinode* unlinknode;
	struct infochunk unlinkchunk;
	unsigned int flag = 1; 
	int j;
	int blocks = (inodes + msg.inum)->addrs[0];
	int p = pread(fd2, &ichunk, BSIZE, (blocks + 3)*BSIZE);
	int inum = -1;
	int found = 0;
	if (p != -1) {
		for (i = 0; i < 64 && found == 0; i++){	
			if (strcmp(ichunk.inf[i].name, msg.name) == 0){
				inum = ichunk.inf[i].inum;
				found = 1;
			}
		}
	}
	if(found == 1){
		i--;
	}
    flag = flag << inum%32;     
    flag = ~flag;           
    bm->inodes[inum/32] = bm->inodes[inum/32] & flag;
	if (p == -1 || inum == -1) {
		res.rc = -1;
		UDP_Write(fd, &client, (char*) &res, sizeof(response));
	}
	else{
		unlinknode = (inodes + inum);
		if (unlinknode->type == MFS_DIRECTORY) {
			pread(fd2, &unlinkchunk, BSIZE, (unlinknode->addrs[0] + 3)*BSIZE);;
			int fill = 0;
			for (j = 2; j < 64; j++){
				if (unlinkchunk.inf[j].inum != -1)
					fill = 1;
			}
			if (fill == 1) {
				res.rc = -1;
				UDP_Write(fd, &client, (char*) &res, sizeof(response));
			} else { 
				ichunk.inf[i].inum = -1;
				pwrite(fd2, &ichunk, BSIZE, ( 3 + blocks) * BSIZE);
			}
		}else {
			for (j = 0; j < NDIRECT; j++) {
				if (unlinknode->addrs[j] != 0) {
					unsigned int flag = 1;  
    				flag = flag << unlinknode->addrs[j]%32;     
      				flag = ~flag;           
      				bm->data[inum/32] = bm->data[inum/32] & flag; 
				}
			}
			ichunk.inf[i].inum = -1;
			pwrite(fd2, &ichunk, BSIZE, (blocks + 3)*BSIZE);
		} 
		res.rc = 0;
		UDP_Write(fd, &client, (char*) &res, sizeof(response));	
	}

}
int main(int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s [portnum] [file-system-image]\n", argv[0]);
		exit(1);
	}

	port = atoi(argv[1]);
	fd = UDP_Open(port);
	
	if ((UDP_FillSockAddr(&client, "localhost", port)) == -1){
		printf("Error\n");
		exit(1);
	}

	if(access(argv[2], F_OK) != -1) { 
		fd2 = open(argv[2], O_RDWR);	
	} else {
		 fd2 = open(argv[2], O_CREAT | O_RDWR);
	}
	if (fd < 0 || fd2 < 0)
		return -1;

	int p = pread(fd2, header_blocks, 3*BSIZE, 0);

	if (p == 0) {
      	bm->data[0] = bm->data[0] | 1;
      	bm->inodes[0] = bm->inodes[0] | 1;

		inodes->type = MFS_DIRECTORY;
		sb->size = 1028;
		sb->nblocks = 1024; 
		sb->ninodes = 64;
		inodes->addrs[0] = 0;

		struct infochunk home;		
		strncpy(home.inf[0].name, "." , 1);
		home.inf[0].inum = 0;
		strncpy(home.inf[1].name, "..", 2);
		home.inf[1].inum = 0;
		for (i = 2; i < 64; i++){
			home.inf[i].inum = -1;
		}
		pwrite(fd2, &home, BSIZE, 3 * BSIZE);
	}
	while (1) {
		if(UDP_Read(fd, &client, (char*) &msg, sizeof(message)) == -1){
			printf("Error\n");
			return 1;
		}
		if (strcmp(msg.cmd, "lookup") == 0) {
			lookup();
		} else if (strcmp(msg.cmd, "stat") == 0) {
			stat();
		}else if (strcmp(msg.cmd, "write") == 0) {
			writes();
		} else if (strcmp(msg.cmd, "read") == 0) {
			reads();
		} else if (strcmp(msg.cmd, "create") == 0) {
			create();
		} else if (strcmp(msg.cmd, "unlink") == 0) {
			unlinks();
		}else if (strcmp(msg.cmd, "shutdown") == 0) {
			res.rc = 0;
			fsync(fd2);
			UDP_Write(fd, &client, (char*) &res, sizeof(response));
			UDP_Close(fd);
			exit(0);
		} else {
			printf("Unknown Command\n");
		}
	}
	exit(0);
}