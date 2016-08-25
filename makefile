httpserver.exe: httpserver.o serve.o
	gcc httpserver.o serve.o -o httpserver -s -pthread
httpserver.o: httpserver.c httpserver.h
	gcc httpserver.c -c
serve.o: serve.c httpserver.h
	gcc serve.c -c
clean:
	del serve.o httpserver.o
