#include "cs537.h"
#include "request.h"
#include <pthread.h>
#include <unistd.h>

// 
// server.c: A very, very simple web server
//
// To run:
//  server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// CS537: Parse the new arguments too


typedef struct buffer{
    int connfd;
    struct buffer* next;
}buffer;

buffer *head;
buffer *tail;
int buffersize;
int total;
pthread_cond_t fill, empty;
pthread_mutex_t lock;

int enqueue(int connfd){
    buffer *newbuffer;
    newbuffer = malloc(sizeof(buffer));
    if(head == NULL){
        newbuffer->connfd = connfd;
        newbuffer->next = NULL;
        total++;
        head = newbuffer;
        tail = head;
    }else{
        //queue is full
        if(total == buffersize){
            return -1;
        }else{
            newbuffer->connfd = connfd;
            newbuffer->next = NULL;
            tail->next = newbuffer;
            tail = newbuffer;
            total++;            
        }
    }
    return 0;
}

int dequeue (){
    //queue is empty
    int connfd;
    if(head == NULL){
        return -1;
    }else{
        connfd = head->connfd;
        buffer *temp;
        temp = head;
        head = temp->next;
        free(temp);
        total--;
    }
    return connfd;
}

void getargs(int *port, int *threads, int *buffersize, int argc, char *argv[])
{
    if (argc != 4) {
	fprintf(stderr, "Usage: %s <port>, <threads>, <buffersize>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *buffersize = atoi(argv[3]);

}
void worker_threads(void *arg){
    while(1){
        pthread_mutex_lock(&lock);
        int connfd;
        connfd = dequeue();
        while(connfd == -1){
            pthread_cond_wait(&fill, &lock);
            connfd = dequeue();

        }
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&lock);

        requestHandle(connfd);
        Close(connfd);
    }
}
    

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, threads;
    struct sockaddr_in clientaddr;

    getargs(&port, &threads, &buffersize, argc, argv);

    // 
    // CS537: Create some threads...
    //
    pthread_t p[threads];
    pthread_cond_init(&fill, NULL);
    pthread_cond_init(&empty, NULL);
    pthread_mutex_init(&lock, NULL);


    int i;
    for(i = 0; i< threads; i++){
        if((pthread_create(&p[threads], NULL, &worker_threads, NULL)) != 0){
            printf("can't create thread\n");
            exit(1); 
        }
    }
    listenfd = Open_listenfd(port);

    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
    pthread_mutex_lock(&lock);

    while((enqueue(connfd)) == -1){
        pthread_cond_wait(&empty, &lock);
    }
    pthread_cond_signal(&fill);
    pthread_mutex_unlock(&lock);


    // 
	// CS537: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work.
	// 
	
    }

}


    


 
