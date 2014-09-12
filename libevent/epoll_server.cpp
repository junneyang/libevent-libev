#ifndef  C_EPOLL_SERVER_H
#define  C_EPOLL_SERVER_H

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <pthread.h>

#define _MAX_SOCKFD_COUNT 65535

class CEpollServer
{
        public:
                CEpollServer();
                ~CEpollServer();

                bool InitServer(const char* chIp, int iPort);
                void Listen();
                static void ListenThread( void* lpVoid );
                void Run();

        private:
                int        m_iEpollFd;
                int        m_isock;
                pthread_t       m_ListenThreadId;// 

};

#endif

using namespace std;

CEpollServer::CEpollServer()
{
}

CEpollServer::~CEpollServer()
{
    close(m_isock);
}

bool CEpollServer::InitServer(const char* pIp, int iPort)
{
    m_iEpollFd = epoll_create(_MAX_SOCKFD_COUNT);

    //
    int opts = O_NONBLOCK;
    if(fcntl(m_iEpollFd,F_SETFL,opts)<0)
    {
        printf("!\n");
        return false;
    }

    m_isock = socket(AF_INET,SOCK_STREAM,0);
    if ( 0 > m_isock )
    {
        printf("socket error!\n");
        return false;
}

sockaddr_in listen_addr;
    listen_addr.sin_family=AF_INET;
    listen_addr.sin_port=htons ( iPort );
    listen_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    listen_addr.sin_addr.s_addr=inet_addr(pIp);

    int ireuseadd_on = 1;//
    setsockopt(m_isock, SOL_SOCKET, SO_REUSEADDR, &ireuseadd_on, sizeof(ireuseadd_on) );

    if ( bind ( m_isock, ( sockaddr * ) &listen_addr,sizeof ( listen_addr ) ) !=0 )
    {
        printf("bind error\n");
        return false;
    }

    if ( listen ( m_isock, 20) <0 )
    {
        printf("listen error!\n");
        return false;
    }
    else
    {
        printf("...\n");
    }

    // epoll
    if ( pthread_create( &m_ListenThreadId, 0, ( void * ( * ) ( void * ) ) ListenThread, this ) != 0 )
    {
        printf("Server !!!");
        return false;
    }
}
// 
void CEpollServer::ListenThread( void* lpVoid )
{
    CEpollServer *pTerminalServer = (CEpollServer*)lpVoid;
    sockaddr_in remote_addr;
    int len = sizeof (remote_addr);
    while ( true )
    {
        int client_socket = accept (pTerminalServer->m_isock, ( sockaddr * ) &remote_addr,(socklen_t*)&len );
        if ( client_socket < 0 )
        {
            printf("Server Accept!, client_socket: %d\n", client_socket);
            continue;
        }
        else
        {
            struct epoll_event    ev;
            ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
            ev.data.fd = client_socket;     //socket
            epoll_ctl(pTerminalServer->m_iEpollFd, EPOLL_CTL_ADD, client_socket, &ev);
        }
    }
}

void CEpollServer::Run()
{
    while ( true )
    {
        struct epoll_event    events[_MAX_SOCKFD_COUNT];
        int nfds = epoll_wait( m_iEpollFd, events,  _MAX_SOCKFD_COUNT, -1 );
        for (int i = 0; i < nfds; i++)
        {
            int client_socket = events[i].data.fd;
            char buffer[1024];//1024
            memset(buffer, 0, 1024);
            if (events[i].events & EPOLLIN)//
            {
                int rev_size = recv(events[i].data.fd,buffer, 1024,0);
                if( rev_size <= 0 )
                {
                    cout << "recv error: recv size: " << rev_size << endl;
                    struct epoll_event event_del;
                    event_del.data.fd = events[i].data.fd;
                    event_del.events = 0;
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, event_del.data.fd, &event_del);
                }
                else
                {
                    printf("Terminal Received Msg Content:%s\n",buffer);
                    struct epoll_event    ev;
                    ev.events = EPOLLOUT | EPOLLERR | EPOLLHUP;
                    ev.data.fd = client_socket;     //socket
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, client_socket, &ev);
                }
            }
else if(events[i].events & EPOLLOUT)//
            {
                char sendbuff[1024];
                sprintf(sendbuff, "Hello, client fd: %d\n", client_socket);
                int sendsize = send(client_socket, sendbuff, strlen(sendbuff)+1, MSG_NOSIGNAL);
                if(sendsize <= 0)
                {
                    struct epoll_event event_del;
                    event_del.data.fd = events[i].data.fd;
                    event_del.events = 0;
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, event_del.data.fd, &event_del);
                }
                else
                {
                    printf("Server reply msg ok! buffer: %s\n", sendbuff);
                    struct epoll_event    ev;
                    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP;
                    ev.data.fd = client_socket;     //socket
                    epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, client_socket, &ev);
                }
            }
            else
            {
                cout << "EPOLL ERROR\n" <<endl;
                epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
            }
        }
    }
}


using namespace std;

int main()
{
        CEpollServer  theApp;
        theApp.InitServer("127.0.0.1", 8000);
        theApp.Run();

        return 0;
}



