#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>

#define HTTPREQSIZE 4096
#define KEEPALIVE_TIMEOUT 10 // seconds
#define SERVER_NAME "stupid-sexy-server"

struct threadhandle{
	struct threadhandle *next;
};
struct threaddata{
	int sock,*abortthread;
	struct sockaddr_in6 sockaddr;
	pthread_mutex_t mutex;
};

int doublenewline(unsigned char*,int);
void serve(void*);
void newconn(struct threadhandle**,HANDLE,int*,int,struct sockaddr_in6*);
void cleanhandles(struct threadhandle**);
BOOL WINAPI handler(DWORD);
