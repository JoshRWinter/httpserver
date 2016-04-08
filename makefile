httpserver.exe: httpserver.o serve.o resource.res
	gcc httpserver.o serve.o resource.res -o httpserver.exe -s -lws2_32
httpserver.o: httpserver.c httpserver.h
	gcc httpserver.c -c
serve.o: serve.c httpserver.h
	gcc serve.c -c
resource.res: resource.rc icon.ico
	windres -i resource.rc -J rc -o resource.res -O coff
clean:
	del httpserver.exe serve.o resource.res httpserver.o