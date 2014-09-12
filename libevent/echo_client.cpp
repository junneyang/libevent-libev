//#include "stdafx.h"
#include "event2/event.h"
#include "event2/util.h"
#include<stdlib.h>
#include<malloc.h>

#include <errno.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <sys/resource.h>
#include <pthread.h>
#include <vector>

#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>
#include <fstream>
#include <streambuf>
#include <string>
#include <cstring>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <algorithm>
#include <pthread.h>
#include <unistd.h>

#define ECHO_PORT   8888
#define ECHO_SERVER "127.0.0.1"

struct echo_context{
    struct event_base *base;
    struct event *event_write;
    struct event *event_read;
    const char * echo_contents;
    int echo_contents_len;
    int recved;
};

void write_cb(evutil_socket_t sock, short flags, void * args)
{
    struct echo_context *ec = (struct echo_context *)args; 

    int ret = send(sock, ec->echo_contents, ec->echo_contents_len, 0);
    printf("connected, write to echo server: %d\n", ret);
    event_add(ec->event_read, 0);
}

void read_cb(evutil_socket_t sock, short flags, void * args)
{
    struct echo_context *ec = (struct echo_context *)args; 
    char buf[128];
    int ret = recv(sock, buf, 128, 0);
    
    printf("read_cb, read %d bytes\n", ret);
    if(ret > 0)
    {
        ec->recved += ret;
        buf[ret] = 0;
        printf("recv:%s\n", buf);
    }
    else if(ret == 0)
    {
        printf("read_cb connection closed\n");
        event_base_loopexit(ec->base, NULL);
        return;
		/*int ret = send(sock, "abc", 3, 0);
		printf("write to echo server: %d\n", ret);
		event_add(ec->event_read, 0);*/
    }
    if(ec->recved < ec->echo_contents_len)
    {
        event_add(ec->event_read, 0);
		printf("what?????\n");
    }
}

static evutil_socket_t make_tcp_socket()
{
    int on = 1;
    evutil_socket_t sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    evutil_make_socket_nonblocking(sock);
#ifdef WIN32
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (const char *)&on, sizeof(on));
#else
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on));
#endif

    return sock;
}

static void echo_client(struct event_base *base, const char *host, unsigned short port)
{
    evutil_socket_t sock = make_tcp_socket();
    struct sockaddr_in serverAddr;
    struct event * ev_write = 0;
    struct event * ev_read = 0;
    struct timeval tv={10, 0};
    struct echo_context *ec = (struct echo_context*)calloc(1, sizeof(struct echo_context));
    
    memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(host);

    connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    ev_write = event_new(base, sock, EV_WRITE, write_cb, (void*)ec);
    ev_read = event_new(base, sock, EV_READ , read_cb, (void*)ec);

    ec->event_write = ev_write;
    ec->event_read = ev_read;
    ec->base = base;
    ec->echo_contents = strdup("echo client tneilc ohce\n");
    ec->echo_contents_len = strlen(ec->echo_contents);
    ec->recved = 0;

    event_add(ev_write, &tv);
}

int main(int argc, char** argv)
{
	struct event_base * base = 0;

	base = event_base_new();
	echo_client(base, argv[1], (unsigned short)(atoi(argv[2])));
	event_base_dispatch(base);
	event_base_free(base);

	return 0;
}

