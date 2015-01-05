all:  libmfs.so 

libmfs.so: mfs.c udp.c server.c client.c
	gcc -c -fpic mfs.c -Wall -Werror 
	gcc -c -fpic udp.c -Wall -Werror 
	gcc server.c -o server udp.o -Wall -Werror
	gcc client.c -o client udp.o -Wall -Werror
	gcc -shared -o libmfs.so mfs.o udp.o

clean:
	rm -rf *.o *.so 
