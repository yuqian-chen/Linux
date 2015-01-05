all: libmem1.so libmem2.so libmem3.so

libmem1.so: mem1.c
	gcc -c -fpic mem1.c -Wall -Werror -pthread
	gcc -shared -o libmem1.so mem1.o

libmem2.so: mem2.c
	gcc -c -fpic mem2.c -Wall -Werror
	gcc -shared -o libmem2.so mem2.o

libmem3.so: mem3.c
	gcc -c -fpic mem3.c -Wall -Werror
	gcc -shared -o libmem3.so mem3.o

clean:
	rm -rf *.so *.o