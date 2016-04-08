#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define HTTPREQSIZE 4096
#define KEEPALIVE_TIMEOUT 10 // seconds
#define SERVER_NAME "stupid-sexy-server"

struct threadhandle{
	HANDLE h;
	struct threadhandle *next;
};
struct threaddata{
	int sock,*abortthread;
	struct sockaddr_in6 sockaddr;
	HANDLE mutex;
};

int doublenewline(unsigned char*,int);
DWORD WINAPI serve(void*);
void newconn(struct threadhandle**,HANDLE,int*,int,struct sockaddr_in6*);
void cleanhandles(struct threadhandle**);
BOOL WINAPI handler(DWORD);