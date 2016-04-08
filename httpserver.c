#include "httpserver.h"

void usage(char **argv){
	fprintf(stderr,"Usage: %s [-p port] [-d dir]\n-p: The port that %s should listen on (default: 80)\n-d: The root directory to serve files from (default: \".\\root\")\n",argv[0],argv[0]);
	exit(1);
}
int main(int argc,char **argv){
	WSADATA wsa;
	int scan,sock=INVALID_SOCKET,abortthread=0;
	struct sockaddr_in6 scanaddr,sockaddr;
	int len=sizeof(struct sockaddr_in6),stop;
	unsigned short port=80;
	char *rootdir="root",option;
	u_long mode=1;
	struct threadhandle *threadhandlehead=NULL;
	HANDLE mutex;
	char **oldargv=argv;
	SMALL_RECT rect={0,0,150,45};
	HANDLE stdouthandle;
	COORD bufsize={151,300};
	
	// size console window
	stdouthandle=GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleScreenBufferSize(stdouthandle,bufsize);
	SetConsoleWindowInfo(stdouthandle,TRUE,&rect);
	
	// parse cmd line args
	while((option=getopt(argc,argv,"p:d:h"))!=-1){
		switch(option){
			case 'p':
				if(1!=sscanf(optarg,"%hu",&port)){
					usage(oldargv);
				}
				break;
			case 'd':
				rootdir=optarg;
				break;
			case '?':
				usage(oldargv);
				break;
			case 'h':
				usage(oldargv);
				break;
		}
	}
	if(_chdir(rootdir)){
		puts("Error: need root directory");
		sleep(2);
		return 1;
	}
	system("title Press [ESC] to exit -- (JOSH WINTER'S SUPER SEXY HTTP SERVER)");
	
	mutex=CreateMutex(NULL,FALSE,NULL);
	SetConsoleCtrlHandler(handler,TRUE);
	WSAStartup(MAKEWORD(1,1),&wsa);
	memset(&scanaddr,0,sizeof(struct sockaddr_in6));
	scanaddr.sin6_family=AF_INET6;
	scanaddr.sin6_port=htons(port);
	scanaddr.sin6_addr=in6addr_any;
	scan=socket(AF_INET6,SOCK_STREAM,0);
	ioctlsocket(scan,FIONBIO,&mode);
	mode=0;
	if(setsockopt(scan,IPPROTO_IPV6,27,(char*)&mode,4))
		printf("failed with error %d\n",WSAGetLastError());
	if(bind(scan,(struct sockaddr*)&scanaddr,sizeof(struct sockaddr_in6))){
		printf("Error: unable to bind to port %hu.\nMake sure no other server is running on that port.\nWSAE: %d",port,WSAGetLastError());
		closesocket(scan);
		WSACleanup();
		sleep(2);
		return 1;
	}
	listen(scan,100);
	printf("[root dir: '%s' port: %hu -- ready]\n",rootdir,port);
	for(;;){
		stop=0;
		sock=INVALID_SOCKET;
		while(sock==INVALID_SOCKET){
			Sleep(30);
			if(_kbhit()){
				char key=getch();
				if(key==27){
					stop=1;
					break;
				}
				else if(key=='\r'){
					system("cls");
					printf("[root dir: '%s' port: %hu -- ready]\n",rootdir,port);
				}
			}
			sock=accept(scan,(struct sockaddr*)&sockaddr,&len);
		}
		if(stop)break;
		newconn(&threadhandlehead,mutex,&abortthread,sock,&sockaddr);
	}
	puts("Waiting for outstanding connections to close...");
	OpenMutex(SYNCHRONIZE,FALSE,NULL);
	abortthread=1;
	ReleaseMutex(mutex);
	while(threadhandlehead!=NULL)cleanhandles(&threadhandlehead); // wait for outstanding connections to finish up
	closesocket(scan);
	WSACleanup();
	CloseHandle(mutex);
	puts("Exiting...");
	sleep(1);
	return 0;
}
void cleanhandles(struct threadhandle **threadhandlehead){
	struct threadhandle *current=*threadhandlehead,*prev=NULL;
	while(current!=NULL){
		if(!WaitForSingleObject(current->h,0)){
			if(!CloseHandle(current->h))fprintf(stderr,"\7Couldn't close handle\n");
			if(prev!=NULL)prev->next=current->next;
			else *threadhandlehead=current->next;
			struct threadhandle *temp=current->next;
			free(current);
			current=temp;
			continue;
		}
		prev=current;
		current=current->next;
	}
}
void newconn(struct threadhandle **threadhandlehead,HANDLE mutex,int *abortthread,int sock,struct sockaddr_in6 *sockaddr){
	DWORD tid;
	HANDLE h;
	cleanhandles(threadhandlehead);
	struct threaddata *td=malloc(sizeof(struct threaddata));
	td->sock=sock;
	td->sockaddr=*sockaddr;
	td->mutex=mutex;
	td->abortthread=abortthread;
	h=CreateThread(NULL,0,serve,td,0,&tid);
	struct threadhandle *th=malloc(sizeof(struct threadhandle));
	th->h=h;
	th->next=*threadhandlehead;
	*threadhandlehead=th;
}
int closeconnections(HANDLE mutex,int *abortthread){
	int val;
	OpenMutex(SYNCHRONIZE,FALSE,NULL);
	val=*abortthread;
	ReleaseMutex(mutex);
	return val;
}
int doublenewline(unsigned char *data,int len){
	int i;
	for(i=3;i<len;++i) // search for "r\n\r\n"
		if(data[i]=='\n'&&data[i-1]=='\r'&&data[i-2]=='\n'&&data[i-3]=='\r')
			return 1;
	return 0;
}
BOOL WINAPI handler(DWORD type){
	switch(type){
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			ungetch(27); // esc key
			return TRUE;
	}
	return FALSE;
}