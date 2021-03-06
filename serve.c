#include "httpserver.h"
#include <ctype.h>

char *getcurrenttime(char *currenttime,pthread_mutex_t mutex){
	pthread_mutex_lock(&mutex); // localtime() is NOT thread safe, must lock with mutexes
	time_t rawtime;
	struct tm *timeinfo;
	rawtime=time(NULL);
	timeinfo=localtime(&rawtime);
	sprintf(currenttime,"%02d:%02d:%02d %d-%02d-%02d",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec,timeinfo->tm_year+1900,timeinfo->tm_mon+1,timeinfo->tm_mday);
	pthread_mutex_unlock(&mutex);
	return currenttime;
}
int is_directory(const char *path) {
   struct stat statbuf;
   if (stat(path,&statbuf)!=0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}
FILE *openresource(int sock,pthread_mutex_t mutex,char *resource,char **code,char *connectto,int sesh){ // resolve resource name
	int i;
	FILE *res;
	int rlen;
	int foundperiod=0;
	char currenttime[26];

	if(strstr(resource,"..")||strstr(resource,":")||strstr(resource,"%")||strstr(resource,"$"))strcpy(resource,"lalalala"); // bad stuff

	if(!strcmp(resource,""))strcpy(resource,"index.html");
	else if(is_directory(resource)){ // is 'resource' a directory?
		strcat(resource,"/index.html");
	}

	rlen=strlen(resource); // convert everything to lowercase
	for(i=0;i<rlen;++i)
		resource[i]=tolower(resource[i]);

	res=fopen(resource,"rb");
	if(!res){
		*code="404 Not Found";
		printf("[%s (%d) - %s] Requested entity not found: %s\n",connectto,sesh,getcurrenttime(currenttime,mutex),resource);
		strcpy(resource,"404page.html");
		res=fopen(resource,"rb");
		if(!res){
			// create an empty 404page.html
			FILE *file=fopen("404page.html","w");
			char *notfound="<!Doctype html>\n<html><head><title>404</title></head><body><h2>404 Page Not Found</h2></body></html>";
			fwrite(notfound,sizeof(char),strlen(notfound),file);
			fclose(file);
			return fopen("404page.html","rb");
		}
	}
	else{
		printf("[%s (%d) - %s] Requested entity: %s\n",connectto,sesh,getcurrenttime(currenttime,mutex),resource);
		*code="200 OK";
	}
	return res;
}
void getfileext(char *resource,char **ext){
	int i;
	int rlen;
	rlen=strlen(resource);
	for(i=rlen-1;resource[i]!='.';--i){
		if(i==0){
			*ext="none";
			return;
		}
	}
	*ext=resource+i+1;
}
void getcontenttype(char *resource,char **contenttype){
	char *ext;
	getfileext(resource,&ext);
	if(!strcmp(ext,"html"))*contenttype="text/html";
	else if(!strcmp(ext,"css"))*contenttype="text/css";
	else if(!strcmp(ext,"js"))*contenttype="application/javascript";
	else if(!strcmp(ext,"ico"))*contenttype="image/x-icon";
	else if(!strcmp(ext,"jpg")||!strcmp(ext,"jpeg"))*contenttype="image/jpeg";
	else if(!strcmp(ext,"png"))*contenttype="image/png";
	else if(!strcmp(ext,"gif"))*contenttype="image/gif";
	else if(!strcmp(ext,"c"))*contenttype="text/plain";
	else if(!strcmp(ext,"webm"))*contenttype="video/webm";
	else if(!strcmp(ext,"mp4"))*contenttype="video/mp4";
	else *contenttype="application/octet-stream";
}
void* serve(void *p){
	char httprequest[HTTPREQSIZE],*contenttype,*path,resource[255],*proto,*code,*connectto,currenttime[26],response[4096],*resdata,fixedresdata[4096],connecteraddress[47];
	u_long mode=0;
	int reslen,result,i,sock,ishtml;
	struct sockaddr_in6 sockaddr;
	unsigned bytestr,responselen;
	FILE *res;
	int initialtime;
	pthread_mutex_t mutex;
	int *abortthread;
	int sesh;

	struct threaddata *td=p;
	sock=td->sock;
	sesh=td->sesh;
	sockaddr=td->sockaddr;
	mutex=td->mutex;
	abortthread=td->abortthread;
	free(td);
	getnameinfo((struct sockaddr*)&sockaddr,sizeof(struct sockaddr_in6),connecteraddress,47,NULL,0,NI_NUMERICHOST);
	if(strstr(connecteraddress,".")){ // ipv4 mapped ipv6 addr
		connectto=connecteraddress+7;
	}
	else{ // ipv6 addr
		connectto=connecteraddress;
	}
	printf("[%s (%d) - %s] Connected\n",connectto,sesh,getcurrenttime(currenttime,mutex));
	initialtime=time(NULL);
	for(;;){
		int stop=0;
		memset(httprequest,0,HTTPREQSIZE);
		bytestr=0;
		mode=1;
		if(-1==ioctl(sock,FIONBIO,&mode))printf("last error: %d",errno);
		while(!doublenewline(httprequest,bytestr)){
			usleep(2000);
			if(time(NULL)-initialtime>KEEPALIVE_TIMEOUT||closeconnections(mutex,abortthread)){
				printf("[%s (%d) - %s] Keepalive timeout\n",connectto,sesh,getcurrenttime(currenttime,mutex));
				stop=1;
				break;
			}
			result=recv(sock,httprequest+bytestr,HTTPREQSIZE-bytestr,0);
			if(result>0)bytestr+=result;
			else if(result==0){
				printf("[%s (%d) - %s] Network error receiving http request: %d\n",connectto,sesh,getcurrenttime(currenttime,mutex),errno);
				close(sock);
				return NULL;
			}
		}
		if(stop)break;
		initialtime=time(NULL);
		mode=0;
		if(-1==ioctl(sock,FIONBIO,&mode))printf("last error: %d",errno);

		if(httprequest[0] != 'G' || httprequest[1] != 'E' || httprequest[2] != 'T' || httprequest[3] != ' '){
			// not implemented.
			printf("[%s (%d) - %s] Not implemented\n",connectto,sesh,getcurrenttime(currenttime,mutex));
			char *ni = "501 Not Implemented\r\n\r\n";
			bytestr=0;
			while(bytestr!=strlen(ni)){
				int result=send(sock,ni+bytestr,strlen(ni)-bytestr,0);
				if(result<0){
					printf("[%s (%d) - %s] Network error serving response: %d\n",connectto,sesh,getcurrenttime(currenttime,mutex),errno);
					close(sock);
					return NULL;
				}
				bytestr+=result;
			}
			break;
		}

		path=httprequest+5; // separate out path (resource);
		// get rid of preceding slashes
		while(path[0]=='/'||path[0]=='\\')++path;
		for(i=0;;++i){
			if(path[i]==' '){
				path[i]=0;
				break;
			}
		}

		proto=httprequest+strlen(httprequest)+1; // separate out proto ('HTTP/1.1' or 'HTTP/1.0' or other)
		for(i=0;;++i){
			if(proto[i]=='\r'){
				proto[i]=0;
				break;
			}
		}

		strcpy(resource,path);
		FILE *res=openresource(sock,mutex,resource,&code,connectto,sesh);
		getcontenttype(resource,&contenttype);
		if(!strcmp("text/html",contenttype)){ // gets treated differently, will try to look for server side includes
			reslen=processhtml(&resdata,res);
			ishtml=1;
		}
		else{
			fseek(res,0,SEEK_END);
			reslen=ftell(res);
			fseek(res,0,SEEK_SET);
			resdata=fixedresdata;
			ishtml=0;
		}
		sprintf(response,"%s %s\r\nServer: "SERVER_NAME"\r\nContent-Type: %s\r\nContent-Length: %u\r\n\r\n",proto,code,contenttype,reslen);
		responselen=strlen(response);
		bytestr=0;
		while(bytestr!=responselen){
			result=send(sock,response+bytestr,responselen-bytestr,0);
			if(result<0){
				printf("[%s (%d) - %s] Network error serving length of %s: %d",connectto,sesh,getcurrenttime(currenttime,mutex),resource,errno);
				close(sock);
				return NULL;
			}
			bytestr+=result;
		}
		bytestr=0;
		while(!feof(res)){
			int bytesread;
			if(ishtml){
				bytesread=reslen;
			}
			else{
				bytesread=fread(resdata,1,4096,res);
			}
			bytestr=0;
			while(bytestr!=bytesread){
				result=send(sock,resdata+bytestr,bytesread-bytestr,0);
				if(result<0){
					int errorcode;
					if(errno!=EWOULDBLOCK){
						fclose(res);
						printf("[%s (%d) - %s] Network error serving %s: %d\n",connectto,sesh,getcurrenttime(currenttime,mutex),resource,errorcode);
						close(sock);
						return NULL;
					}
				}
				else bytestr+=result;
			}
			if(ishtml){
				free(resdata);
				break;
			}
		}
		fclose(res);
		printf("[%s (%d) - %s] Served %s (%s) (%u)\n",connectto,sesh,getcurrenttime(currenttime,mutex),resource,contenttype,reslen);
	}
	printf("[%s (%d) - %s] Disconnected\n",connectto,sesh,getcurrenttime(currenttime,mutex));
	close(sock);
	return NULL;
}
