#include <stdio.h>
#include <pthread.h> // we import the pthread library
#include "request.h"
#include "io_helper.h"

//
// ./wserver [-d <basedir>] [-p <portnum>] [-t threads] [-b buffersize]
// 

char default_root[] = ".";
int *buffer = NULL;   
int count = 0;   // count indicate the number of requests waiting in queue
int use_ptr = 0;
int fill_ptr = 0;
int buffersize = 1;   // default value of buffersize is 1
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;      // pthread condition variables and mutex
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// functions for queue operations

void put_in_buffer(int value) {
	buffer[fill_ptr] = value;
	fill_ptr = (fill_ptr + 1) %buffersize;
	count++;
} 
int get_from_buffer() {
	int tmp = buffer[use_ptr];
	use_ptr = (use_ptr + 1) % buffersize;
	count--;
	return tmp;
}


// the consummer function

void *connection_handler() {
	puts("Running thread");
	while(1) {
		pthread_mutex_lock(&mutex);
		while (count == 0) {
			pthread_cond_wait(&fill, &mutex);
		}
    	
    	int conn_fd = get_from_buffer();
		pthread_cond_signal(&empty);
		pthread_mutex_unlock(&mutex);
		request_handle(conn_fd);
		close_or_die(conn_fd);
	}
	
}



int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
	int threads = 1;
	
   // the parsing of the provided arguments

    while ((c = getopt(argc, argv, "d:p:t:b:")) != -1)
	switch (c) {
	case 'd':
	    root_dir = optarg;
	    break;
	case 'p':
	    port = atoi(optarg);
		
	    break;
	case 't':
		threads = atoi(optarg);
	    break;
	case 'b':
		buffersize = atoi(optarg);
	    break;

	default:
	    fprintf(stderr, "usage: wserver [-d basedir] [-p port] [-t threads] [-b buffersize]\n");
	    exit(1);
	}

    // run out of this directory
    chdir_or_die(root_dir);
    // memory allocation for the buffer 
	buffer = (int*) malloc(sizeof(int)*buffersize);
	// we create the threads
	pthread_t thrs[threads];
	for(int i=0;i<threads;i++) {
    pthread_create( &thrs[i], NULL , connection_handler , 1);
	}

    // file descriptor to listen on the port
    int listen_fd = open_listen_fd_or_die(port);
    

	// the producer threads
	while (1) {
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);
		int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
		
		pthread_mutex_lock(&mutex); 
		
		while (count == buffersize ) {
			
			pthread_cond_wait(&empty, &mutex);
		}
		put_in_buffer(conn_fd);
		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&mutex);
    }
    return 0;
}
