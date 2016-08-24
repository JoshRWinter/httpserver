httpserver.exe: httpserver.o serve.o resource.res
	gcc httpserver.o serve.o -o httpserver -s
httpserver.o: httpserver.c httpserver.h
	gcc httpserver.c -c
serve.o: serve.c httpserver.h
	gcc serve.c -c
clean:
	del httpserver serve.o httpserver.o
