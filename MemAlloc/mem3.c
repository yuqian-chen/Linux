#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include "mem.h"

typedef struct freelist{
	int status;
	int size;
	struct freelist* next;
}freelist;

freelist* head = NULL;
int allocated = 0;


int
Mem_Init(int sizeOfRegion){
	int pagesize = getpagesize();
	int initsize = sizeOfRegion;

	if(allocated != 0 || sizeOfRegion <= 0){
		fprintf(stderr, "ERROR!\n");
		return -1;
	}

	if(initsize % pagesize != 0){
		initsize += (pagesize - (initsize % pagesize));
	}

	int fd = open("/dev/zero", O_RDWR);
	void* allocptr = mmap(NULL, initsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if(allocptr == MAP_FAILED){
		perror("mmap");
		exit(1);
	}
	close(fd);

	head = (freelist*)allocptr;
	head->size = initsize - sizeof(freelist);
	head->status = 0;
	
	allocated = 1;
	return 0;
}


void*
Mem_Alloc(int size){
	/* Declare variables */
	int allocsize = size;
	int found = 0;
	int tmp;
	freelist* allocptr;
	freelist* memptr = head;

	/* Check for sanity of size - Return NULL when appropriate */
	if(allocsize < 0){
		return NULL;
	}
	/* Round up size to a multiple of 8 */
	if(allocsize % 8 != 0){
		allocsize += (8 - (allocsize % 8));
	}
	/* First Fit allocate process */
	while(found == 0 && memptr != NULL){
		if(memptr->status == 0 && memptr->size >= allocsize){
			found = 1;
			if(memptr->size > allocsize + sizeof(freelist)){
				allocptr = memptr;
				tmp = memptr->size;
				memptr = (void*)memptr + sizeof(freelist) + allocsize;
				memptr->size = tmp - allocsize - sizeof(freelist);
				memptr->status = 0;
				memptr->next = allocptr->next;
				allocptr->size = allocsize;
				allocptr->status = 1;
				allocptr->next = memptr;
				allocptr = allocptr + 1;
				break;
			}
			else{
				allocptr = memptr;
				allocptr->size = allocsize;
				allocptr->status = 1;
				allocptr = allocptr + 1;
				break;
			}
		}
		memptr = memptr->next;
	}
	if(found == 0){
		return NULL;
	}
	return allocptr;
}

int
Mem_Free(void *ptr){
	/* Declare variables */
    pthread_mutex_t lock;
    pthread_mutex_lock(&lock);
	freelist* freeptr = (freelist*)ptr - 1;
	freelist* memptr = head;
	freelist* prevptr = head;
	int found = 0;

	/* Return -1 if ptr is NULL or if ptr is not pointing to the first byte of a busy block */
	if(ptr == NULL){
        pthread_mutex_unlock(&lock);
		return -1;
	}
	/* Traverse to find the target block that will be freed */
	while(found == 0 && memptr != NULL){
        
		if(memptr == freeptr && memptr->status == 1){
			/* mark the block as free */
			memptr->status = 0;
			found = 1;
			/* coalesce if previous neighbour is free */
			if(memptr != prevptr && prevptr->status == 0){
				prevptr->size += sizeof(freelist) + memptr->size;
				prevptr->next = memptr->next;
				memptr = prevptr;
			}
			/* coalesce if next neighbour is free */
			freelist* nextptr = memptr->next;
			if(nextptr != NULL && nextptr->status == 0){
				memptr->size += sizeof(freelist) + nextptr->size;
				memptr->next = nextptr->next;
			}
		}
		/* update the tmp pointer and tmpPrv pointer */
		prevptr = memptr;
		memptr = memptr->next;
	}
    pthread_mutex_unlock(&lock);
	if(found == 0){
		return -1;
	}
	return 0;
}

int
Mem_Available(){
	freelist* memptr = head;
	int available = 0;
	while(memptr != NULL){
		if(memptr->size % 2 == 0){
			available += memptr->size;
		}
		memptr = memptr->next;
	}
	return available;
}


void
Mem_Dump(){

}