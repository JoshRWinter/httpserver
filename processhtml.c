#include "httpserver.h"

static void *fill_error(char **resourcedata,int *len){
	char *errorstring="<!Doctype html>\n<html><head><title>Internal server error</title></head><body><p>Internal server error</p></body></html>";
	*len=strlen(errorstring);
	*resourcedata=malloc(*len);
	memcpy(*resourcedata,errorstring,*len);
}

static void append(char **resourcedata,int *resourcedatalen,int startingindex,char *data,int datalen){
	int freespace=(*resourcedatalen)-startingindex;
	while(datalen>freespace){
		(*resourcedatalen)*=2;
		freespace=(*resourcedatalen)-startingindex;
		*resourcedata=realloc(*resourcedata,*resourcedatalen);
	}
	memcpy((*resourcedata)+startingindex,data,datalen);
}

// note: this function allocates memory that needs to be freed by its caller (resourcedata)
int processhtml(char **resourcedata,FILE *res){
	// at this point "res" fd will be opened, still needs to be read and processed.
	// "res" will be fclose-ed later by the caller of this function

	// determine length of html file being processed
	fseek(res,0,SEEK_END);
	int filedatalen=ftell(res);
	fseek(res,0,SEEK_SET);

	// read all data from html file being processed
	char *filedata=malloc(sizeof(char)*filedatalen);
	fread(filedata,sizeof(char),filedatalen,res);

	// html file will be processed and stuffed into "resourcedata" which has yet to be allocated
	const int CHUNK=1024;
	*resourcedata=malloc(CHUNK);
	int resourcedatalen=CHUNK;

	int bytesprocessed=0; // total stream bytes processed. will be the return value of this function
	int bookmark=0; // where the last loop left off in the html file being processed

	// begin loop looking for server side includes
	char *includesubstring;
	while(includesubstring=strstr(filedata+bookmark,"####")){
		int includepos=(includesubstring-filedata);

		// copy everything up to this point (since last run of this loop) into stream
		append(resourcedata,&resourcedatalen,bytesprocessed,filedata+bookmark,includepos-bookmark);
		bytesprocessed+=includepos-bookmark;
		bookmark=includepos;

		// copy server side include string into proper c string
		char path[1025];
		int pathindex=0;
		int pathpos=includepos+4;
		while(pathindex<1024&&pathpos<filedatalen&&filedata[pathpos]!='\n'){
			path[pathindex]=filedata[pathpos];
			++pathpos;
			++pathindex;
		}
		path[pathindex]=0;

		bookmark+=strlen(path)+4; // plus 4 to get rid of the "####"

		// now try to load server side include file that now exists in "path"
		FILE *includefile=fopen(path,"rb");
		if(!includefile){ // can't find it? give up.
			free(*resourcedata);
			free(filedata);
			int len;
			fill_error(resourcedata,&len);
			return len;
		}

		// determine length of server side include file
		fseek(includefile,0,SEEK_END);
		int includefilelen=ftell(includefile);
		fseek(includefile,0,SEEK_SET);

		// allocate and read
		char *includefiledata=malloc(includefilelen);
		fread(includefiledata,sizeof(char),includefilelen,includefile);
		fclose(includefile);

		// copy into stream
		append(resourcedata,&resourcedatalen,bytesprocessed,includefiledata,includefilelen);
		bytesprocessed+=includefilelen;
		free(includefiledata);
	}
	append(resourcedata,&resourcedatalen,bytesprocessed,filedata+bookmark,filedatalen-bookmark);
	bytesprocessed+=filedatalen-bookmark;
	free(filedata);
	return bytesprocessed;
}
