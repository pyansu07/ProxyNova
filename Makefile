CC=g++
CFLAGS= -g -Wall 
LDFLAGS= -lws2_32 -lpthread

all: proxy.exe

proxy.exe: proxy_server_with_cache.c proxy_parse.c proxy_parse.h
	$(CC) $(CFLAGS) -c proxy_parse.c
	$(CC) $(CFLAGS) -c proxy_server_with_cache.c
	$(CC) $(CFLAGS) -o proxy.exe proxy_parse.o proxy_server_with_cache.o $(LDFLAGS)

clean:
	del /F /Q proxy.exe *.o

zip:
	powershell Compress-Archive -Path proxy_server_with_cache.c,README,Makefile,proxy_parse.c,proxy_parse.h -DestinationPath ass1.zip