#include "httpserver.h"

int controlc=0; // set by control-c handler

void usage(char **argv){
	printf("Usage: %s [-p port] [-d dir] [-f]\n-p: The port that %s should listen on (default: 80)\n-d: The root directory to serve files from (default: \"./root\")\n-b: detach from the controlling terminal (daemonize)\n    stdout will be redirected to ~/httplog.txt\n",argv[0],argv[0]);
	exit(1);
}
int main(int argc,char **argv){
	int scan,sock=-1,abortthread=0;
	struct sockaddr_in6 scanaddr,sockaddr;
	int len=sizeof(struct sockaddr_in6),stop;
	unsigned short port=80;
	char *rootdir="root",option;
	u_long mode=1;
	struct threadhandle *threadhandlehead=NULL;
	pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
	char **oldargv=argv;
	char logpath[1024];
	
	// parse cmd line args
	while((option=getopt(argc,argv,"p:d:hb"))!=-1){
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
			case 'b': // run in background
				if(fork())return 0;
				strcpy(logpath,getenv("HOME"));
				strcat(logpath,"/httplog.txt");
				fprintf(stderr,"%s\n",logpath);
				freopen(logpath,"w",stdout);
				break;
		}
	}
	if(chdir(rootdir)){
		fprintf(stderr,"Error: need root directory\n");
		return 1;
	}

	signal(SIGINT,signalcatcher); // register signal catcher to catch control-c
	signal(SIGHUP,signalcatcher); // register signal catcher to catch SIGHUP
	signal(SIGTERM,signalcatcher); // register signal catcher to catch SIGTERM
	signal(SIGPIPE,sigpipetrap); // register signal catcher to catch SIGPIPE
	
	memset(&scanaddr,0,sizeof(struct sockaddr_in6));
	scanaddr.sin6_family=AF_INET6;
	scanaddr.sin6_port=htons(port);
	scanaddr.sin6_addr=in6addr_any;
	scan=socket(AF_INET6,SOCK_STREAM,0);
	ioctl(scan,FIONBIO,&mode);
	mode=0;
	/*if(setsockopt(scan,IPPROTO_IPV6,27,(char*)&mode,4))
		printf("failed with error %d\n",WSAGetLastError());*/ //don't think this is needed on linux, all sockets are dual stack by default
	if(bind(scan,(struct sockaddr*)&scanaddr,sizeof(struct sockaddr_in6))){
		fprintf(stderr,"Error: unable to bind to port %hu.\nMake sure no other server is running on that port. E: %d\n",port,errno);
		close(scan);
		return 1;
	}
	listen(scan,100);

	printf("[root dir: '%s' port: %hu -- ready]\n",rootdir,port);
	for(;;){
		stop=0;
		sock=-1;
		while(sock==-1){
			usleep(30000); // 30 millis
			if(controlc){ // interrupt (SIGINT) was received, need to exit
				stop=1;
				break;
			}
			sock=accept(scan,(struct sockaddr*)&sockaddr,&len);
		}
		if(stop)break;
		newconn(&threadhandlehead,mutex,&abortthread,sock,&sockaddr);
	}
	puts("Waiting for outstanding connections to close...");
	pthread_mutex_lock(&mutex);
	abortthread=1;
	pthread_mutex_unlock(&mutex);
	while(threadhandlehead!=NULL)joinall(&threadhandlehead); // wait for outstanding connections to finish up
	close(scan);
	puts("Exiting...");
	return 0;
}
void joinall(struct threadhandle **threadhandlehead){
	struct threadhandle *current=*threadhandlehead,*prev=NULL;
	while(current!=NULL){
		if(pthread_kill(current->h,0)!=0){ // if thread has finished
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
void newconn(struct threadhandle **threadhandlehead,pthread_mutex_t mutex,int *abortthread,int sock,struct sockaddr_in6 *sockaddr){
	static int sesh=100; // session id
	pthread_t h;
	joinall(threadhandlehead);
	struct threaddata *td=malloc(sizeof(struct threaddata));
	td->sesh=sesh;
	sesh++;
	if(sesh>1000000)sesh=100;
	td->sock=sock;
	td->sockaddr=*sockaddr;
	td->mutex=mutex;
	td->abortthread=abortthread;
	pthread_create(&h,NULL,serve,td);
	struct threadhandle *th=malloc(sizeof(struct threadhandle));
	th->h=h;
	th->next=*threadhandlehead;
	*threadhandlehead=th;
}
int closeconnections(pthread_mutex_t mutex,int *abortthread){
	int val;
	pthread_mutex_lock(&mutex);
	val=*abortthread;
	pthread_mutex_unlock(&mutex);
	return val;
}
int doublenewline(unsigned char *data,int len){
	int i;
	for(i=3;i<len;++i) // search for "r\n\r\n"
		if(data[i]=='\n'&&data[i-1]=='\r'&&data[i-2]=='\n'&&data[i-3]=='\r')
			return 1;
	return 0;
}
void signalcatcher(int sig){
	printf("\n----- Interrupt received\n");
	controlc=1;
}
void sigpipetrap(int sig){
	fprintf(stderr,"----------- ERR SIGPIPE\n");
}
