/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


#define MAX_LINE 16384
static const int MY_PORT = 9999;
static const int LISTEN_QUENE = 16;

 

/**
 * 将字符变成大写
 */
char DoUpper(char c){
	if ('a' <= c && c <= 'z'){
		c = c - ('a' - 'A');
	}
	return c;
}

 

/**

 * 处理输入数据的回调函数

 */

void ReadCallBack(struct bufferevent *pBuffEnt, void *pContext)

{

    struct evbuffer* pInstream = bufferevent_get_input(pBuffEnt);

    struct evbuffer* pOutstream = bufferevent_get_output(pBuffEnt);

 

    char *szLine;

    size_t n;

    // 注意，返回的line是在堆上分配的内存，用完后马上需要清理，否则产生内存泄漏

    while ((szLine = evbuffer_readln(pInstream, &n, EVBUFFER_EOL_LF)))

    {

        // 将当前行的字符串转换

        for (int i = 0; i < n; ++i)

        {

            szLine[i] = DoUpper(szLine[i]);

        }

 

        // 将当前数据输出给客户端

        evbuffer_add(pOutstream, szLine, n);

        evbuffer_add(pOutstream, "\n", 1);

        free(szLine);

    }

 

    if (evbuffer_get_length(pInstream) >= MAX_LINE) {

        /* Too long; just process what there is and go on so that the buffer

         * doesn't grow infinitely long. */

        char buf[1024];

        while (evbuffer_get_length(pInstream)) {

            int n = evbuffer_remove(pInstream, buf, sizeof(buf));

            for (int i = 0; i < n; ++i)

            {

                buf[i] = DoUpper(buf[i]);

            }

            evbuffer_add(pOutstream, buf, n);

        }

        evbuffer_add(pOutstream, "\n", 1);

    }

}

 

void ErrorCallBack(struct bufferevent *bev, short error, void *ctx)

{

    if (error & BEV_EVENT_EOF) {

        /* connection has been closed, do any clean up here */

        printf("ErrorCallBack, connection closed\n");

    } else if (error & BEV_EVENT_ERROR) {

        perror("ErrorCallBack");

    } else if (error & BEV_EVENT_TIMEOUT) {

        /* must be a timeout event handle, handle it */

        /* ... */

    }

    bufferevent_free(bev);

}

 

/**

 * 回调函数会接受三个参数

 * listener 注册的fd

 * event    注册的事件

 * arg      注册时的参数

 */

void DoAccept(evutil_socket_t nListenSock, short event, void *pArg)

{

    // 获取链接的fd

    struct sockaddr_storage oAddr;

    socklen_t nAddrLen = sizeof(oAddr);

    int nConnSock = accept(nListenSock, (struct sockaddr*)&oAddr, &nAddrLen);

    if (nConnSock < 0) {

        perror("accept");

    } else if (nConnSock > FD_SETSIZE) {

        close(nConnSock);

    } else {

        evutil_make_socket_nonblocking(nConnSock); // 设置为非堵塞的socket

 

        // 获取传入的参数——event base,自对象在DoAccpet中穿件，用于存放所有的fd

        struct event_base *pEventBase = (struct event_base*)pArg;

        // 创建一个缓冲事件，缓冲事件，顾名思义，就是当数据缓冲到一定程度，才触发，而不是只要有数据就触发

        struct bufferevent* pBuffEnt = bufferevent_socket_new(pEventBase, nConnSock, BEV_OPT_CLOSE_ON_FREE);

        bufferevent_setcb(pBuffEnt, ReadCallBack, NULL, ErrorCallBack, NULL);

        // “程度”通过高/低水位来设定

        bufferevent_setwatermark(pBuffEnt, EV_READ, 0, MAX_LINE);

        // 必须调用这句，否则enabled == false

        bufferevent_enable(pBuffEnt, EV_READ|EV_WRITE);

    }

}

 

void Run(void)

{

    // 设置地址，此服务器监听在40713上

    struct sockaddr_in oAddr;

    oAddr.sin_family = AF_INET;

    oAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    oAddr.sin_port = htons(MY_PORT);

 

    // 创建监听socket

    evutil_socket_t nListenSock = socket(AF_INET, SOCK_STREAM, 0);

    evutil_make_socket_nonblocking(nListenSock);

 

    // 将监听socket设置为地址重用

    int nOne = 1;

    setsockopt(nListenSock, SOL_SOCKET, SO_REUSEADDR, &nOne, sizeof(nOne));

 

    // 绑定端口

    if (bind(nListenSock, (struct sockaddr*)&oAddr, sizeof(oAddr)) < 0)

    {

        perror("bind");

        return;

    }

 

    // 开始监听

    if (listen(nListenSock, LISTEN_QUENE)<0)

    {

        perror("listen");

        return;

    }

 

    // 创建事件的集合对象

    struct event_base *pEventBase = event_base_new();

    if (NULL == pEventBase)

    {

        perror("event_base creating failed");

        return; /*XXXerr*/

    }

    // 将listen socket fd的read事件添加到事件集合中

    // EV_PERSIST设置监听socket的读取事件（新的连接）持续发生

    struct event* pListenEvent = event_new(pEventBase, nListenSock, EV_READ|EV_PERSIST, DoAccept, (void*)pEventBase);

 

    // 注册监听事件

    event_add(pListenEvent, NULL);

 

    // 开始主程序的循环

    event_base_dispatch(pEventBase);

}

 

int main(int c, char **v)

{

    // 设置标准输出stdout没有缓冲，直接输出任何异常

    setvbuf(stdout, NULL, _IONBF, 0);

 

    Run();

    return 0;

}