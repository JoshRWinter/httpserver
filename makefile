httpserver.exe: httpserver.o serve.o processhtml.o
	gcc httpserver.o serve.o processhtml.o -o httpserver -s -pthread
httpserver.o: httpserver.c httpserver.h
	gcc httpserver.c -c
serve.o: serve.c httpserver.h
	gcc serve.c -c
processhtml.o: processhtml.c httpserver.h
	gcc processhtml.c -c
clean:
	rm *.o
