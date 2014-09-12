// App01.cpp : 定义控制台应用程序的入口点。
//
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/thread.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/bufferevent_struct.h>
#include <thread>
#include <iostream>
#include <vector>

/* 设置计数，只允许echo_write_cb调用一次 */
static int count = 1;
static std::vector<struct bufferevent *> g_bev;//bufferevent缓冲区

/*当有数据可读的时候，会调用这个函数 */
//读取回调函数
static void echo_read_cb(struct bufferevent *bev, void *ctx)
{
    printf("读：echo_read_cb is called\n");
    char bufs[1000] = {0};
    bufferevent_read(bev, bufs, sizeof(bufs));
    std::cout<< bufs <<"\n";
    return;

    printf("读：echo_read_cb is called\n");
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);
    
    size_t len = evbuffer_get_length(input);
    printf("evbuffer input length is: %lu\n", (unsigned long)len);

    //evbuffer_add_buffer(output, input);

    char buf[1024];
    int n;
    n = evbuffer_remove(input, buf, sizeof(buf));
    printf("copy bytes == %d\n", n);
    printf("copy buf: %s\n", buf);
}

//写入回调函数
static void echo_write_cb(struct bufferevent *bev, void *ctx)
{

    printf("写：echo_write_cb is called\n");
    char sendbuffer[] = "yes, i recv your message!\n";    
    return;

    struct evbuffer *output = bufferevent_get_output(bev);    
    
    if(count == 1)
    {
        int result = evbuffer_add(output, sendbuffer, strlen(sendbuffer));
        printf("evbuffer_add result = %d\n", result);    
    }
    
    count++;
    int len = evbuffer_get_length(output);
    evbuffer_drain(output, len);    
}


/*当客户端结束的时候，肯定会调用这个函数  */
//事件回调函数
static void echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
    printf("状态：echo_event_cb is called\n");
    if(events & BEV_EVENT_ERROR)
        perror("Error from bufferevent");
    if(events & BEV_EVENT_EOF | BEV_EVENT_ERROR)
    {
        bufferevent_free(bev);
        printf("bufferevent_free is called\n");
    }
    printf("-------------------------------\n\n");
    
}    


//client连接回调
static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, 
    struct sockaddr*address, int socklen, void *ctx)
{
    printf("连接：Accept_conn_cb is called\n");
    struct event_base *base = evconnlistener_get_base(listener);//返回监听器关联的event_base
    struct bufferevent *bev = bufferevent_socket_new(base, fd,//创建基于套接字的bufferevent
        BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
    if (!bev) 
    {
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(base);
        return ;
    }
    g_bev.push_back(bev);

    bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, "Server参数");//设置读回调，错误事件回调,null表示禁止回调，cbarg向回调传递参数
    
    bufferevent_enable(bev, EV_READ|EV_WRITE);    //开启缓冲区上的读，写事件
}

//连接监听器错误回调函数
static void accept_error_cb(struct evconnlistener *listener, void *ctx)
{    
    printf("监听错误：Accept_error_cb is called\n");
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr, "Got an error");
    
    event_base_loopexit(base, NULL);
    
}

struct event_base *g_base;

int main(int argc, char **argv)
{
    struct evconnlistener *listener;
    struct sockaddr_in sin;
    
    int port = 9527;
    
    if(argc > 1)
    {
        port = atoi(argv[1]);
    }
    if(port <= 0 || port > 65535)
    {
        puts("Invalid port");
        return 1;
    }
#ifdef WIN32
    WSADATA wsa_data;
    WSAStartup(0x0201, &wsa_data);//启动异步socket
#endif
    if(-1 == evthread_use_windows_threads())//event多线程支持
        return false;

    g_base = event_base_new();//创建一个event_base
    if(!g_base)
    {
        puts("could't open event_base");
        return 1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(0);
    sin.sin_port = htons(port);
    //分配返回一个监听器对象，有连接会回调accept_conn_cb函数,绑定iP,port
    listener = evconnlistener_new_bind(g_base, accept_conn_cb, NULL,
        LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
        (struct sockaddr*)&sin, sizeof(sin));
    if(!listener)
    {
        perror("could't not create listener");
        return 1;
    }
    
    evconnlistener_set_error_cb(listener, accept_error_cb);//侦听错误
    std::thread th([]
    {
        event_base_dispatch(g_base);//程序进入无线循环，等待就绪事件并执行事件处理
    });
    int count = 0;
    for(;;)
    {
        if(!g_bev.empty())
        {
            for(auto it : g_bev)
            {
                char msg[50] = {0};
                sprintf(msg, "欢迎！Server send!%d", count);
                bufferevent_write(it, msg, 50);//想缓冲区添加数据
            }
            count++;
        }
        Sleep(50);
    }
    th.join();
    return 0;
}

