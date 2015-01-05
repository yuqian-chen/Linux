#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "mem.h"

/* Each chunk of memory is 16 bytes,
   use one bit to represent the status of memory units.
   0 -> free
   1 -> 16 bytes*/

#define SHIFTL(x,n) (x << n)

int allocated = 0;
int bitmap_size;
int free_num;
int units_num;
char* bitmap_ptr;
void* alloc_ptr;

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
	alloc_ptr = mmap(NULL, initsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if(alloc_ptr == MAP_FAILED){
		perror("mmap");
		exit(1);
	}
	close(fd);

	bitmap_size = initsize / (16 * 8);
	initsize -= bitmap_size;
	free_num = initsize;
	bitmap_ptr = (char*)alloc_ptr;
	alloc_ptr += bitmap_size;
	units_num = initsize/16;

	allocated = 1;
	return 0;
}


void*
Mem_Alloc(int size){
	int i, j;
	char* tmp = bitmap_ptr;
	void* retptr;

	if(size < 0 || size != 16){
		return NULL;
	}
	/* Find a 0 bit(free) in bitmap and return the related memory pointer*/
	/* Also update the number of available mem byte*/
	for(i = 0; i < bitmap_size; i++){
		for(j = 0; j < 8; j++){
			if((i * 8 + j) >= units_num){
				return NULL;
			}
			else if(SHIFTL(0x01,j) & *tmp){
				continue;
			}
			else{
				*tmp = *tmp | SHIFTL(0x01,j);
				retptr = alloc_ptr + (((i * 8) + j) * 16);
				free_num -= 16;
				return retptr;
			}
		}
		tmp++;
	}
	return NULL;
}

int
Mem_Free(void *ptr){
    pthread_mutex_t lock;
    pthread_mutex_lock(&lock);
	int i, j;
	char* tmp;
	if(ptr == NULL || ptr < alloc_ptr
	  || ptr > alloc_ptr + (units_num * 16)
	  || (ptr - alloc_ptr) % 16 != 0){
        pthread_mutex_unlock(&lock);
		return -1;
	}
	/* Find the related bit in bitmap and set it to 0(free)*/
	i = (ptr - alloc_ptr) / (16 * 8);
	j = (ptr - alloc_ptr) % (16 * 8);
	tmp = bitmap_ptr + i;
	*tmp = *tmp & SHIFTL(0xFE,j);
	/* Update the number of available mem byte*/
	free_num += 16;
    pthread_mutex_unlock(&lock);
    return 0;


}

int
Mem_Available(){
	return free_num;
}

void
Mem_Dump(){

}