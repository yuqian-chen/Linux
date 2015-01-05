#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "mem.h"
#include <pthread.h>

#define SHIFTL(x,n) (x << n)

/* Each chunk of memory is 16 bytes,
   use two bit to represent the status of memory units.
   00 -> free
   01 -> 16 bytes
   10 -> 80 bytes
   11 -> 256 bytes */

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

	bitmap_size = initsize / (16 * 4);
	initsize -= bitmap_size;
	free_num = initsize;
	bitmap_ptr = alloc_ptr;
	alloc_ptr += bitmap_size; 
	units_num = initsize/16;

	allocated = 1;
	return 0;
}


void*
Mem_Alloc(int size){
	int i = 0;
	int j = 0;
	char* tmp = bitmap_ptr;
	void* retptr;
	char* mapptr;
	int tmpj = 0;
	int count = 0;

	if(size < 0 || size > free_num || (size != 16
		&& size != 80 && size != 256)){
		return NULL;
	}
	/* Search a free chunk by check every two bits. During the search, jump over any allocated chunk
	(i.e. if find a 11 then jump over next 32 bit because all of them must be allocated)
	If we find a 00 bits, then we also need to check if there are enough space to allocate
	aquired bytes(i.e. if aquire 80 bytes, then we need to check if there are 4 contigious 
	free chunks after the 00 bits */
	while(1){
		int alloc_num = 2;
		if((i * 4 + j * (1/2)) >= units_num){
			return NULL;
		}
		else if((SHIFTL(0x03,j) & *tmp) == SHIFTL(0x03,j)){
			count = 0;
			alloc_num = 32;
		}
		else if((SHIFTL(0x01,j) & *tmp) == SHIFTL(0x01,j)){
			count = 0;
			alloc_num = 2;
		}
		else if((SHIFTL(0x02,j) & *tmp) == SHIFTL(0x02,j)){
			count = 0;
			alloc_num = 10;
		}
		else{
			if(count == 0){
				retptr = alloc_ptr + (((i * 8) + j) * 8);
				mapptr = tmp;
				tmpj = j;
				count = size/16;
			}
			count--;
			if(count == 0){
				if(size == 16){
					*mapptr = *mapptr | SHIFTL(0x01,tmpj);
					free_num -= 16;
				}
				else if(size == 80){
					*mapptr = *mapptr | SHIFTL(0x02,tmpj);
					free_num -= 80;
				}
				else if(size == 256){
					*mapptr = *mapptr | SHIFTL(0x03,tmpj);
					free_num -= 256;
				}
				return retptr;
			}
		}
		if(j + alloc_num < 8){
			j += alloc_num;
		}
		else{
			i += (j + alloc_num) / 8;
			tmp += (j + alloc_num) / 8;
			j = (j + alloc_num) % 8;
		}
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
	/* Find the related bit in bitmap and set it to free*/
	i = (ptr - alloc_ptr) / (16 * 4);
	j = ((ptr - alloc_ptr) % (16 * 4))/8;
	tmp = bitmap_ptr + i;
	/* Check the size of freed block and update
	   the number of available mem byte*/
	if((SHIFTL(0x03,j) & *tmp) == SHIFTL(0x03,j)){
		free_num += 256;
		*tmp = *tmp & ~SHIFTL(0x03,j);
	}
	else if((SHIFTL(0x01,j) & *tmp) == SHIFTL(0x01,j)){
		free_num += 16;
		*tmp = *tmp & ~SHIFTL(0x01,j);
	}
	else if((SHIFTL(0x02,j) & *tmp) == SHIFTL(0x02,j)){
		free_num += 80;
		*tmp = *tmp & ~SHIFTL(0x02,j);
	}
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