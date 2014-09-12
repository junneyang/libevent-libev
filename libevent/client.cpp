// App02.cpp : 定义控制台应用程序的入口点。
//
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <iostream>
#include <thread>
#include <atomic>

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

std::atomic_bool write_over;

void eventcb(struct bufferevent *bev, short events, void *ptr)
{
    if (events & BEV_EVENT_CONNECTED) 
    {
         /* We're connected to 127.0.0.1:8080.   Ordinarily we'd do
            something here, like start reading or writing. */
    } 
    else if (events & BEV_EVENT_ERROR) 
    {
         /* An error occured while connecting. */
    }
}

//接收Server来的消息
static void conn_readcb(struct bufferevent *bev, void *user_data)//arg1:发生了事件的bufferevent，最后一个是用户提供的参数，可以通过这个向回调传递参数
{
    char buf[50] = {0};
    int len = bufferevent_read(bev, buf, sizeof(buf));
    std::cout<<"来自Server:"<< buf << std::endl;
}

static void conn_writecb(struct bufferevent *bev, void *user_data)
{
    write_over = true;
    //int ret = bufferevent_write(bev, "我是一个客户端！", 20);
}
   
struct event_base *g_base;//管理事件


int main()
{
    write_over = false;
    struct bufferevent *bev;
    struct sockaddr_in sin;
#ifdef WIN32
    WSADATA wsa_data;
    WSAStartup(0x0201, &wsa_data);
#endif
    //event支持多线程的初始化函数
    if(-1 == evthread_use_windows_threads())
        return false;

    g_base = event_base_new();

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(0x7f000001); /* 127.0.0.1 */
    sin.sin_port = htons(9527); /* Port 9527 */

    bev = bufferevent_socket_new(g_base, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);//创建基于套接字的bufferevent

 //   bufferevent_setcb(bev, NULL, NULL, eventcb, NULL);

    if (bufferevent_socket_connect(bev,//connect套接字
        (struct sockaddr *)&sin, sizeof(sin)) < 0) 
    {
        /* Error starting connection */
        std::cout<<"连接失败！\n";
        bufferevent_free(bev);
        return -1;
    }
    bufferevent_setcb(bev, conn_readcb, /*conn_writecb*/nullptr, eventcb, "回调参数");//修改回调
    bufferevent_enable(bev, EV_READ | EV_WRITE );

    std::thread th([]
    {
        event_base_dispatch(g_base);//循环处理事件
    });
    
    int ret = bufferevent_write(bev, "我是客户端-1", 20);

    std::cout<<"输入你想发送的内容：\n";
    int count = 0;
    while(1)
    {
    //    if(write_over)
        {
            write_over = false;
            char msg[50] = {0};
            sprintf(msg, "第%d次发送信息！", ++count);
            bufferevent_write(bev, msg, strlen(msg) + 1);
            Sleep(500);
        }
    //    Sleep(50);
    }

    th.join();
    std::cout<<" over!\n";
    getchar();
    return 0;
}

