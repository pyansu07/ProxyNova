#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_BYTES 4096
#define MAX_SIZE 200*(1<<20)
#define MAX_ELEMENT_SIZE 10*(1<<20)


typedef struct cache_element cache_element;

struct cache_element{
    char* data;
    int len;
    char* url;        
    time_t lru_time_track;
    cache_element* next;    
};


cache_element* find(char* url);
int add_cache_element(char* data,int size,char* url);
void remove_cache_element();

int port_number = 8080;
SOCKET proxy_socketId;
cache_element* head;
int cache_size;    


int sendErrorMessage(SOCKET socket, int status_code)
{
    char str[1024];
    char currentTime[50];
    time_t now = time(0);

    struct tm data;
    gmtime_s(&data, &now);
    strftime(currentTime,sizeof(currentTime),"%a, %d %b %Y %H:%M:%S %Z", &data);

    switch(status_code)
    {
        case 400: _snprintf_s(str, sizeof(str), _TRUNCATE, "HTTP/1.1 400 Bad Request\r\nContent-Length: 95\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n<BODY><H1>400 Bad Rqeuest</H1>\n</BODY></HTML>", currentTime);
                  printf("400 Bad Request\n");
                  send(socket, str, (int)strlen(str), 0);
                  break;

        case 403: _snprintf_s(str, sizeof(str), _TRUNCATE, "HTTP/1.1 403 Forbidden\r\nContent-Length: 112\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H1>403 Forbidden</H1><br>Permission Denied\n</BODY></HTML>", currentTime);
                  printf("403 Forbidden\n");
                  send(socket, str, (int)strlen(str), 0);
                  break;

        case 404: _snprintf_s(str, sizeof(str), _TRUNCATE, "HTTP/1.1 404 Not Found\r\nContent-Length: 91\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H1>404 Not Found</H1>\n</BODY></HTML>", currentTime);
                  printf("404 Not Found\n");
                  send(socket, str, (int)strlen(str), 0);
                  break;

        case 500: _snprintf_s(str, sizeof(str), _TRUNCATE, "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 115\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H1>500 Internal Server Error</H1>\n</BODY></HTML>", currentTime);
                  send(socket, str, (int)strlen(str), 0);
                  break;

        case 501: _snprintf_s(str, sizeof(str), _TRUNCATE, "HTTP/1.1 501 Not Implemented\r\nContent-Length: 103\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Implemented</TITLE></HEAD>\n<BODY><H1>501 Not Implemented</H1>\n</BODY></HTML>", currentTime);
                  printf("501 Not Implemented\n");
                  send(socket, str, (int)strlen(str), 0);
                  break;

        case 505: _snprintf_s(str, sizeof(str), _TRUNCATE, "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 125\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n<BODY><H1>505 HTTP Version Not Supported</H1>\n</BODY></HTML>", currentTime);
                  printf("505 HTTP Version Not Supported\n");
                  send(socket, str, (int)strlen(str), 0);
                  break;

        default:  return -1;
    }
    return 1;
}

SOCKET connectRemoteServer(char* host_addr, int port_num)
{
    SOCKET remoteSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL, *ptr = NULL, hints;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char port_str[6];
    _snprintf_s(port_str, sizeof(port_str), _TRUNCATE, "%d", port_num);

    int iResult = getaddrinfo(host_addr, port_str, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        return INVALID_SOCKET;
    }

    for(ptr=result; ptr != NULL; ptr=ptr->ai_next) {
        remoteSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (remoteSocket == INVALID_SOCKET) {
            printf("Error creating socket: %d\n", WSAGetLastError());
            continue;
        }

        iResult = connect(remoteSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(remoteSocket);
            remoteSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (remoteSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        return INVALID_SOCKET;
    }

    return remoteSocket;
}

int handle_request(SOCKET clientSocket, ParsedRequest *request, char *tempReq)
{
    char *buf = (char*)malloc(sizeof(char)*MAX_BYTES);
    strcpy_s(buf, MAX_BYTES, "GET ");
    strcat_s(buf, MAX_BYTES, request->path);
    strcat_s(buf, MAX_BYTES, " ");
    strcat_s(buf, MAX_BYTES, request->version);
    strcat_s(buf, MAX_BYTES, "\r\n");

    size_t len = strlen(buf);

    if (ParsedHeader_set(request, "Connection", "close") < 0){
        printf("set header key not work\n");
    }

    if(ParsedHeader_get(request, "Host") == NULL)
    {
        if(ParsedHeader_set(request, "Host", request->host) < 0){
            printf("Set \"Host\" header key not working\n");
        }
    }

    if (ParsedRequest_unparse_headers(request, buf + len, (size_t)MAX_BYTES - len) < 0) {
        printf("unparse failed\n");
    }

    int server_port = 80;
    if(request->port != NULL)
        server_port = atoi(request->port);

    SOCKET remoteSocketID = connectRemoteServer(request->host, server_port);

    if(remoteSocketID == INVALID_SOCKET)
        return -1;

    int bytes_send = send(remoteSocketID, buf, (int)strlen(buf), 0);

    ZeroMemory(buf, MAX_BYTES);

    bytes_send = recv(remoteSocketID, buf, MAX_BYTES-1, 0);
    char *temp_buffer = (char*)malloc(sizeof(char)*MAX_BYTES);
    int temp_buffer_size = MAX_BYTES;
    int temp_buffer_index = 0;

    while(bytes_send > 0)
    {
        bytes_send = send(clientSocket, buf, bytes_send, 0);
        
        for(int i=0; i<bytes_send; i++){
            temp_buffer[temp_buffer_index] = buf[i];
            temp_buffer_index++;
        }
        temp_buffer_size += MAX_BYTES;
        temp_buffer = (char*)realloc(temp_buffer, temp_buffer_size);

        if(bytes_send == SOCKET_ERROR)
        {
            printf("Error in sending data to client socket: %d\n", WSAGetLastError());
            break;
        }
        ZeroMemory(buf, MAX_BYTES);

        bytes_send = recv(remoteSocketID, buf, MAX_BYTES-1, 0);
    } 
    temp_buffer[temp_buffer_index]='\0';
    free(buf);
    add_cache_element(temp_buffer, strlen(temp_buffer), tempReq);
    printf("Done\n");
    free(temp_buffer);
    
    closesocket(remoteSocketID);
    return 0;
}

int checkHTTPversion(char *msg)
{
    int version = -1;

    if(strncmp(msg, "HTTP/1.1", 8) == 0)
    {
        version = 1;
    }
    else if(strncmp(msg, "HTTP/1.0", 8) == 0)			
    {
        version = 1;									
    }
    else
        version = -1;

    return version;
}

int handle_client(SOCKET socket)
{
    int bytes_send_client, len;

    char *buffer = (char*)calloc(MAX_BYTES, sizeof(char));
    
    ZeroMemory(buffer, MAX_BYTES);
    bytes_send_client = recv(socket, buffer, MAX_BYTES, 0);
    
    while(bytes_send_client > 0)
    {
        len = strlen(buffer);
        if(strstr(buffer, "\r\n\r\n") == NULL)
        {	
            bytes_send_client = recv(socket, buffer + len, MAX_BYTES - len, 0);
        }
        else{
            break;
        }
    }
    
    char *tempReq = (char*)malloc(strlen(buffer)*sizeof(char)+1);
    strcpy_s(tempReq, strlen(buffer)+1, buffer);
    
    struct cache_element* temp = find(tempReq);

    if(temp != NULL){
        int size = temp->len/sizeof(char);
        int pos = 0;
        char response[MAX_BYTES];
        while(pos < size){
            ZeroMemory(response, MAX_BYTES);
            for(int i=0; i<MAX_BYTES && pos<size; i++){
                response[i] = temp->data[pos];
                pos++;
            }
            send(socket, response, MAX_BYTES, 0);
        }
        printf("Data retrieved from the Cache\n\n");
        printf("%s\n\n", response);
    }
    else if(bytes_send_client > 0)
    {
        len = strlen(buffer);
        ParsedRequest* request = ParsedRequest_create();
        
        if (ParsedRequest_parse(request, buffer, len) < 0) 
        {
            printf("Parsing failed\n");
        }
        else
        {	
            ZeroMemory(buffer, MAX_BYTES);
            if(!strcmp(request->method,"GET"))							
            {
                if(request->host && request->path && (checkHTTPversion(request->version) == 1))
                {
                    bytes_send_client = handle_request(socket, request, tempReq);
                    if(bytes_send_client == -1)
                    {	
                        sendErrorMessage(socket, 500);
                    }
                }
                else
                    sendErrorMessage(socket, 500);
            }
            else
            {
                printf("This code doesn't support any method other than GET\n");
            }
        }
        ParsedRequest_destroy(request);
    }
    else if(bytes_send_client == SOCKET_ERROR)
    {
        printf("Error in receiving from client: %d\n", WSAGetLastError());
    }
    else if(bytes_send_client == 0)
    {
        printf("Client disconnected!\n");
    }

    shutdown(socket, SD_BOTH);
    closesocket(socket);
    free(buffer);
    free(tempReq);
    return 0;
}

int main(int argc, char * argv[]) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    SOCKET client_socketId;
    int client_len;
    struct sockaddr_in server_addr, client_addr;

    if(argc == 2)
    {
        port_number = atoi(argv[1]);
    }
    else
    {
        printf("Too few arguments\n");
        exit(1);
    }

    printf("Setting Proxy Server Port : %d\n",port_number);

    proxy_socketId = socket(AF_INET, SOCK_STREAM, 0);

    if(proxy_socketId == INVALID_SOCKET) {
        printf("Failed to create socket.\n");
        WSACleanup();
        exit(1);
    }

    int reuse = 1;
    if (setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) == SOCKET_ERROR) 
        printf("setsockopt(SO_REUSEADDR) failed\n");

    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(proxy_socketId, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Port is not free\n");
        closesocket(proxy_socketId);
        WSACleanup();
        exit(1);
    }
    printf("Binding on port: %d\n", port_number);

    if(listen(proxy_socketId, SOMAXCONN) == SOCKET_ERROR) {
        printf("Error while Listening !\n");
        closesocket(proxy_socketId);
        WSACleanup();
        exit(1);
    }

    while(1) {
        ZeroMemory(&client_addr, sizeof(client_addr));
        client_len = sizeof(client_addr);

        client_socketId = accept(proxy_socketId, (struct sockaddr*)&client_addr, &client_len);
        if(client_socketId == INVALID_SOCKET) {
            fprintf(stderr, "Error in Accepting connection !\n");
            continue;
        }

        char str[INET_ADDRSTRLEN];
       // inet_ntop(AF_INET, &client_addr.sin_addr, str, INET_ADDRSTRLEN);
        printf("Client is connected with port number: %d and ip address: %s \n", ntohs(client_addr.sin_port), str);

        handle_client(client_socketId);
    }

    closesocket(proxy_socketId);
    WSACleanup();
    return 0;
}

cache_element* find(char* url) {
    cache_element* site = NULL;
    
    if(head != NULL) {
        site = head;
        while (site != NULL) {
            if(!strcmp(site->url, url)) {
                printf("LRU Time Track Before : %lld", site->lru_time_track);
                printf("\nurl found\n");
                site->lru_time_track = time(NULL);
                printf("LRU Time Track After : %lld", site->lru_time_track);
                break;
            }
            site = site->next;
        }       
    } else {
        printf("\nurl not found\n");
    }
    
    return site;
}

void remove_cache_element() {
    cache_element *p, *q, *temp;
    
    if(head != NULL) {
        for (q = head, p = head, temp = head; q->next != NULL; q = q->next) {
            if((q->next->lru_time_track) < (temp->lru_time_track)) {
                temp = q->next;
                p = q;
            }
        }
        if(temp == head) {
            head = head->next;
        } else {
            p->next = temp->next;   
        }
        cache_size = cache_size - (temp->len) - sizeof(cache_element) - strlen(temp->url) - 1;
        free(temp->data);
        free(temp->url);
        free(temp);
    }
}

int add_cache_element(char* data, int size, char* url) {
    int element_size = size + 1 + strlen(url) + sizeof(cache_element);
    if(element_size > MAX_ELEMENT_SIZE) {
        return 0;
    } else {
        while(cache_size + element_size > MAX_SIZE) {
            remove_cache_element();
        }
        cache_element* element = (cache_element*) malloc(sizeof(cache_element));
        element->data = (char*)malloc(size + 1);
        strcpy_s(element->data, size + 1, data);
        element->url = (char*)malloc(1 + (strlen(url) * sizeof(char)));
        strcpy_s(element->url, strlen(url) + 1, url);
        element->lru_time_track = time(NULL);
        element->next = head;
        element->len = size;
        head = element;
        cache_size += element_size;
        return 1;
    }
    return 0;
}
