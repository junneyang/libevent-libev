#include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <arpa/inet.h>
    #include <sys/epoll.h>
    #include <errno.h>

    #define DEFAULT_PORT    1984    //默认端口
    #define BUFF_SIZE       1024    //buffer大小

    #define EPOLL_MAXEVENTS 64      //epoll_wait的最多返回的events个数
    #define EPOLL_TIMEOUT   5000    //epoll_wait的timeout milliseconds

    //函数：设置sock为non-blocking mode
    void setSockNonBlock(int sock) {
        int flags;
        flags = fcntl(sock, F_GETFL, 0);
        if (flags < 0) {
            perror("fcntl(F_GETFL) failed");
            exit(EXIT_FAILURE);
        }
        if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
            perror("fcntl(F_SETFL) failed");
            exit(EXIT_FAILURE);
        }
    }

    int main(int argc, char *argv[]) {

        //获取自定义端口
        unsigned short int port;
        if (argc == 2) {
            port = atoi(argv[1]);
        } else if (argc < 2) {
            port = DEFAULT_PORT;
        } else {
            fprintf(stderr, "USAGE: %s [port]\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        //创建socket
        int sock;
        if ( (sock = socket(PF_INET, SOCK_STREAM, 0)) == -1 ) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }
        printf("socket done\n");

        //in case of 'address already in use' error message
        int yes = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))) {
            perror("setsockopt failed");
            exit(EXIT_FAILURE);
        }

        //设置sock为non-blocking
        setSockNonBlock(sock);

        //创建要bind的socket address
        struct sockaddr_in bind_addr;
        memset(&bind_addr, 0, sizeof(bind_addr));
        bind_addr.sin_family = AF_INET;
        bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);  //设置接受任意地址
        bind_addr.sin_port = htons(port);               //将host byte order转换为network byte order

        //bind sock到创建的socket address上
        if ( bind(sock, (struct sockaddr *) &bind_addr, sizeof(bind_addr)) == -1 ) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        printf("bind done\n");

        //listen
        if ( listen(sock, 5) == -1) {
            perror("listen failed");
            exit(EXIT_FAILURE);
        }
        printf("listen done\n");

        //创建epoll (epoll file descriptor)
        int epfd;
        if ( (epfd = epoll_create(1)) == -1 ) {
            perror("epoll_create failed");
            exit(EXIT_FAILURE);
        }
        //将sock添加到epoll中
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = sock;
        if ( epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &event) == -1 ) {
            perror("epoll_ctl");
            exit(EXIT_FAILURE);
        }

        //初始化epoll_wait的参数
        struct epoll_event events[EPOLL_MAXEVENTS];
        memset(events, 0, sizeof(events));

        //循环侦听
        int conn_sock;
        struct sockaddr_in client_addr;
        socklen_t client_addr_len;
        char client_ip_str[INET_ADDRSTRLEN];
        int res;
        int i;
        char buffer[BUFF_SIZE];
        int recv_size;

        while (1) {

            //每次循环调用依次epoll_wait侦听
            res = epoll_wait(epfd, events, EPOLL_MAXEVENTS, EPOLL_TIMEOUT);
            if (res < 0) {
                perror("epoll_wait failed");
                exit(EXIT_FAILURE);
            } else if (res == 0) {
                fprintf(stderr, "no socket ready for read within %d secs\n", EPOLL_TIMEOUT / 1000);
                continue;
            }

            //检测到res个IO file descriptor的events，loop各个fd进行响应
            for (i = 0; i < res; i++) {
                //events[i]即为检测到的event，域events[i].events表示具体哪些events，域events[i].data.fd即对应的IO fd

                if ( (events[i].events & EPOLLERR) || 
                     (events[i].events & EPOLLHUP) ||
                     (!(events[i].events & EPOLLIN)) ) {
                    //由于events[i].events使用每个bit表示event，因此判断是否包含某个具体事件可以使用`&`操作符
                    //这里判断是否存在EPOLLERR, EPOLLHUP等event
                    fprintf (stderr, "epoll error\n");
                    close (events[i].data.fd);
                    continue;
                }

                //对检测到event的各IO fd进行响应
                if (events[i].data.fd == sock) {

                    //当前fd是server的socket，不进行读而是accept所有client连接请求
                    while (1) {
                        client_addr_len = sizeof(client_addr);
                        conn_sock = accept(sock, (struct sockaddr *) &client_addr, &client_addr_len);
                        if (conn_sock == -1) {
                            if ( (errno == EAGAIN) || (errno == EWOULDBLOCK) ) {
                                //non-blocking模式下无新connection请求，跳出while (1)
                                break;
                            } else {
                                perror("accept failed");
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip_str, sizeof(client_ip_str))) {
                            perror("inet_ntop failed");
                            exit(EXIT_FAILURE);
                        }
                        printf("accept a client from: %s\n", client_ip_str);
                        //设置conn_sock为non-blocking
                        setSockNonBlock(conn_sock);
                        //把conn_sock添加到epoll的侦听中
                        event.events = EPOLLIN;
                        event.data.fd = conn_sock;
                        if ( epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock, &event) == -1 ) {
                            perror("epoll_ctl(EPOLL_CTL_ADD) failed");
                            exit(EXIT_FAILURE);
                        }
                    }

                } else {

                    //当前fd是client连接的socket，可以读(read from client)
                    conn_sock = events[i].data.fd;
                    memset(buffer, 0, sizeof(buffer));
                    if ( (recv_size = recv(conn_sock, buffer, sizeof(buffer), 0)) == -1  && (errno != EAGAIN) ) {
                        //recv在non-blocking模式下，返回-1且errno为EAGAIN表示当前无可读数据，并不表示错误
                        perror("recv failed");
                        exit(EXIT_FAILURE);
                    }
                    printf("recved from conn_sock=%d : %s(%d length string)\n", conn_sock, buffer, recv_size);

                    //立即将收到的内容写回去
                    if ( send(conn_sock, buffer, recv_size, 0) == -1 && (errno != EAGAIN) && (errno != EWOULDBLOCK) ) {
                        //send在non-blocking模式下，返回-1且errno为EAGAIN或EWOULDBLOCK表示当前无可写数据，并不表示错误
                        perror("send failed");
                        exit(EXIT_FAILURE);
                    }
                    printf("send to conn_sock=%d done\n", conn_sock);

                    /*//将当前socket从epoll的侦听中移除(有文章说：关闭con_sock之后，其会自动从epoll中删除，因此此段代码可以省略)
                    if ( epoll_ctl(epfd, EPOLL_CTL_DEL, conn_sock, NULL) == -1 ) {
                        perror("epoll_ctl(EPOLL_CTL_DEL) failed");
                        exit(EXIT_FAILURE);
                    }

                    //关闭连接
                    if ( close(conn_sock) == -1 ) {
                        perror("close failed");
                        exit(EXIT_FAILURE);
                    }
                    printf("close conn_sock=%d done\n", conn_sock);*/
                }
            }

        }

        close(sock);    //关闭server的listening socket
        close(epfd);    //关闭epoll file descriptor

        return 0;
    }


