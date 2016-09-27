#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <netdb.h>

#define HTTPREQSIZE 4096
#define KEEPALIVE_TIMEOUT 10 // seconds
#define SERVER_NAME "stupid-sexy-server" //seen in http response

struct threadhandle{
	pthread_t h;
	struct threadhandle *next;
};
struct threaddata{
	int sesh;
	int sock,*abortthread;
	struct sockaddr_in6 sockaddr;
	pthread_mutex_t mutex;
};

int doublenewline(unsigned char*,int);
int processhtml(char**,FILE*);
void *serve(void*);
void newconn(struct threadhandle**,pthread_mutex_t,int*,int,struct sockaddr_in6*);
void joinall(struct threadhandle**);
int closeconnections(pthread_mutex_t,int*);
void signalcatcher(int);
