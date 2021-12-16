#include <stdio.h>
#include <pthread.h>
#include "request.h"
#include "io_helper.h"

//
// ./wserver [-d <basedir>] [-p <portnum>] [-t threads] [-b buffersize]
// 

char default_root[] = ".";
int *buffer = NULL;
int count = 0;
int use_ptr = 0;
int fill_ptr = 0;
int buffersize = 1;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;



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
	
   
    while ((c = getopt(argc, argv, "d:p:t:b:")) != -1)
	switch (c) {
	case 'd':
	    root_dir = optarg;
	    break;
	case 'p':
	    port = atoi(optarg);
		//atoi converts string to int
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

	buffer = (int*) malloc(sizeof(int)*buffersize);
	//pthread_t thr;
	pthread_t thrs[threads];
	for(int i=0;i<threads;i++) {
    pthread_create( &thrs[i], NULL , connection_handler , 1);
	}

    // now, get to work 
    int listen_fd = open_listen_fd_or_die(port);
    
	while (1) {
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);
		int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
		
		pthread_mutex_lock(&mutex); 
		int waiting = 0;
		while (count == buffersize ) {
			printf(" Max count %d\n", count);
			waiting++;
			pthread_cond_wait(&empty, &mutex);
		}
		put_in_buffer(conn_fd);
		pthread_cond_signal(&fill);
		pthread_mutex_unlock(&mutex);
    }
    return 0;
}


    

/*
Your master thread is then responsible for accepting new HTTP connections over the
 network and placing the descriptor for this connection into a fixed-size buffer;
  in your basic implementation, the master thread should not read from this connection.
   The number of elements in the buffer is also specified on the command line.
    Note that the existing web server has a single thread that accepts a connection 
	and then immediately handles the connection; in your web server, this thread should
	 place the connection descriptor into a fixed-size buffer and return to accepting
	  more connections. 
*/
 
