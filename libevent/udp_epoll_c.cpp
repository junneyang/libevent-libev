/***************************************************************************
file: easysocketbenchmark.cpp
-------------------
begin : 2014/09/01
copyright : (C) 2014 by 
email : yangjun03@baidu.com
***************************************************************************/

/***************************************************************************
* *
* This program is free software; you can redistribute it and/or modify *
* it under the terms of the GNU General Public License as published by *
* the Free Software Foundation; either version 2 of the License, or *
* (at your option) any later version. *
* *
***************************************************************************/

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

static int num=0;
using namespace std;
int Write(int fd,const void *buffer,unsigned int length);
static string request_str;

//int Read(int fd,void *buffer,unsigned int length){
int Read(int fd,char *buffer,unsigned int length){
	/*unsigned int nleft;
	int nread;
	char *ptr;

	ptr = (char *)buffer;
	nleft = length;
	while(nleft > 0){
		if((nread = read(fd, ptr, nleft))< 0){
			if(errno == EINTR)
				nread = 0;
			else
				return -1;
		}else if(nread == 0) {
			break;
		}
		nleft -= nread;
		ptr += nread;
	}
	
	return length - nleft;*/
	int n = 0;
	int nread;
	while ((nread = read(fd, buffer + n, length-1)) > 0) {
		n += nread;
	}
	if (nread == -1 && errno != EAGAIN) {
		perror("read error");
	}
}

int Write(int fd,const void *buffer,unsigned int length){
	unsigned int nleft;
	int nwritten;
	const char *ptr;
	ptr = (const char *)buffer;
	nleft = length;

	while(nleft > 0){
		if((nwritten = write(fd, ptr, nleft))<=0){
			if(errno == EINTR)
				nwritten=0;
			else
				return -1;
		}

		nleft -= nwritten;
		ptr += nwritten;

	}

	return length;
}

int CreateThread(void *(*start_routine)(void *), void *arg = NULL, pthread_t *thread = NULL, pthread_attr_t *pAttr = NULL){
	pthread_attr_t thr_attr;
	if(pAttr == NULL){
		pAttr = &thr_attr;
		pthread_attr_init(pAttr);
		pthread_attr_setstacksize(pAttr, 1024 * 1024); // 1 M
		pthread_attr_setdetachstate(pAttr, PTHREAD_CREATE_DETACHED);
	}
	pthread_t tid;
	if(thread == NULL){
		thread = &tid;
	}

	int r = pthread_create(thread, pAttr, start_routine, arg);
	pthread_attr_destroy(pAttr);
	return r;
}

static int SetRLimit(){
	struct rlimit rlim;
	rlim.rlim_cur = 20480;
	rlim.rlim_max = 20480;

	if (setrlimit(RLIMIT_NOFILE, &rlim) != 0){
		perror("setrlimit");
	}else{
		printf("setrlimit ok\n");
	}

	return 0;
}

int setnonblocking(int sock) {
	int opts;
	opts=fcntl(sock,F_GETFL);
	if(opts<0){
		return -1;
	}

	opts = opts|O_NONBLOCK;
	if(fcntl(sock,F_SETFL,opts)<0){
		return -1;
	}

	return 0;
}

int ConnectToUdperver(const char *host, unsigned short port){
	//int sock = socket(AF_INET, SOCK_DGRAM, 0);
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0){
		perror("socket");
		return -1;
	}

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(host);

	if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		perror("bind");
		close(sock);
		return -1;
	}
	return sock;
}

void *SendThread(void *arg){
	vector<int> sockets;
	sockets = *((vector<int> *)arg);
	int n = 0;
	char data[1024];
	int i = 0;
	//while(1){
	for(vector<int>::iterator itr = sockets.begin(), last = sockets.end(); itr != last; ++itr){
		sprintf(data, "test data %d\n", i++);
		
		ostringstream  request_stream;
		request_stream << "POST " << "/lbs/da/openservice" << " HTTP/1.0\r\n";
		request_stream << "Host:" << "10.81.15.47" << "\r\n";
		request_stream << "Accept: */*\r\n";
		request_stream << "Content-Type: application/json; charset=utf-8\r\n";
		request_stream << "Content-Length: " << request_str.length() << "\r\n";
		request_stream << "Referer: http://10.81.15.47:18080/lbs/da/openservice\r\n\r\n";
		//request_stream << "Connection: close\r\n\r\n";
		request_stream << request_str;
		
		//cout << request_stream.str().c_str() <<endl;
		//n = Write(*itr, "hello", 5);
		n = Write(*itr, request_stream.str().c_str(), strlen(request_stream.str().c_str()));
		
		//printf("socket %d write to server[ret = %d]:%s",*itr, n, data);
		//cout << strlen(request_stream.str().c_str()) <<endl;
		/*const char *buf=request_stream.str().c_str();
		int nwrite, data_size = strlen(buf);
		int n = data_size;
		while (n > 0) {
			nwrite = write(*itr, buf + data_size - n, n);
			if (nwrite < n) {
				if (nwrite == -1 && errno != EAGAIN) {
					perror("write error");
				}
				break;
			}
			n -= nwrite;
		}
		//close(fd);
		//cout << "EPOLLOUT : " << buf <<endl;
		event.data.fd=*itr;
		//设置用于注测的读操作事件
		event.events=EPOLLIN | EPOLLET;
		epoll_ctl(epfd,EPOLL_CTL_MOD,*itr,&event);*/
	}
		//sleep(1);
	//}
}

int main(int argc, char **argv){
	/*std::ifstream t(argv[3]);
	request_str((std::istreambuf_iterator<char>(t)),std::istreambuf_iterator<char>());
	std::cout << "testdata_str:\n" << request_str << "\n";*/
	
	fstream fs( argv[3] ) ; // 创建个文件流对象,并打开"file.txt"
	stringstream ss ;        // 创建字符串流对象
	ss << fs.rdbuf()     ;     // 把文件流中的字符输入到字符串流中
	request_str = ss.str() ;    // 获取流中的字符串
	std::cout << "testdata_str:\n" << request_str << "\n";
	
	//SetRLimit();
	printf("FD_SETSIZE= %d\n", FD_SETSIZE);
	if (argc != 4){
		printf("usage: %s <IPaddress> <PORT> <DATA>\n", argv[0]);
		return 1;
	}
	int epfd = epoll_create(20480); 
	if(epfd < 0){
		perror("epoll_create");
		return 1;
	}

	struct epoll_event event;
	struct epoll_event ev[20480];
	vector<int> sockets;
	for(int i = 0; i < 10; i++){
		int sockfd = ConnectToUdperver(argv[1], (unsigned short)(atoi(argv[2])));
		if(sockfd < 0){
			printf("Cannot connect udp server %s %s\n", argv[1], argv[2]);
			return 1;
		}
		sockets.push_back(sockfd);
		setnonblocking(sockfd);
		event.data.fd = sockfd;
		//event.events = EPOLLIN|EPOLLET|EPOLLOUT;
		event.events = EPOLLIN|EPOLLET;
		if(0 != epoll_ctl(epfd,EPOLL_CTL_ADD,sockfd,&event)){
			perror("epoll_ctl");
		}
	}
	
	if(0 != CreateThread(SendThread, (void *)&sockets)){
		perror("CreateThread");
		return 2;
	}

	int nfds = 0;
	
	int n,nread;
	char buf[BUFSIZ];
	while(1){
		//nfds=epoll_wait(epfd,ev,20480,500);
		nfds=epoll_wait(epfd,ev,20480,-1);
		//cout<<"execute epoll wait....\n";
		if(nfds < 0){
			printf("epoll waite error, exit loop\n");
			break;
		}
		else if(nfds == 0){
			printf("no event, continue...\n");
			continue;
		}
		for(int i = 0; i < nfds; i++){
			if(ev[i].events & EPOLLIN){
				//printf("epoll in event....\n");
				n = 0;
                while ((nread = read(ev[i].data.fd, buf + n, BUFSIZ-1)) > 0) {
                    n += nread;
                }
                if (nread == -1 && errno != EAGAIN) {
                    printf("read error...\n");
                }
				//cout << "read : " << buf <<endl;
                /*event.data.fd = ev[i].data.fd;
                event.events = ev[i].events | EPOLLOUT;
                if (epoll_ctl(epfd, EPOLL_CTL_MOD, ev[i].data.fd, &event) == -1) {
                    printf("epoll_ctl: mod...\n");
                }*/
				//cout << "set socket epollout" <<endl;
				
				ostringstream  request_stream;
				request_stream << "POST " << "/lbs/da/openservice" << " HTTP/1.0\r\n";
				request_stream << "Host:" << "10.81.15.47" << "\r\n";
				request_stream << "Accept: */*\r\n";
				request_stream << "Content-Type: application/json; charset=utf-8\r\n";
				request_stream << "Content-Length: " << request_str.length() << "\r\n";
				request_stream << "Referer: http://10.81.15.47:18080/lbs/da/openservice\r\n\r\n";
				//request_stream << "Connection: close\r\n\r\n";
				request_stream << request_str;
				const char *buf=request_stream.str().c_str();
				
                int nwrite, data_size = strlen(buf);
                n = data_size;
                while (n > 0) {
                    nwrite = write(ev[i].data.fd, buf + data_size - n, n);
                    if (nwrite < n) {
                        if (nwrite == -1 && errno != EAGAIN) {
                            printf("write error...\n");
                        }
                        break;
                    }
                    n -= nwrite;
                }
				num+=1;
				cout<<num<<endl;
				
			}
			/*else if (ev[i].events & EPOLLOUT) {
				//printf("epoll out event....\n");
                ostringstream  request_stream;
				request_stream << "POST " << "/lbs/da/openservice" << " HTTP/1.0\r\n";
				request_stream << "Host:" << "10.81.15.47" << "\r\n";*/
				//request_stream << "Accept: */*\r\n";
				/*request_stream << "Content-Type: application/json; charset=utf-8\r\n";
				request_stream << "Content-Length: " << request_str.length() << "\r\n";
				request_stream << "Referer: http://10.81.15.47:18080/lbs/da/openservice\r\n\r\n";
				//request_stream << "Connection: close\r\n\r\n";
				request_stream << request_str;
				const char *buf=request_stream.str().c_str();
				
                int nwrite, data_size = strlen(buf);
                n = data_size;
                while (n > 0) {
                    nwrite = write(ev[i].data.fd, buf + data_size - n, n);
                    if (nwrite < n) {
                        if (nwrite == -1 && errno != EAGAIN) {
                            printf("write error...\n");
                        }
                        break;
                    }
                    n -= nwrite;
                }
                //close(fd);
				//cout << "EPOLLOUT : " << buf <<endl;
				event.data.fd=ev[i].data.fd;
				//设置用于注测的读操作事件
				event.events=ev[i].events | EPOLLIN;
				epoll_ctl(epfd,EPOLL_CTL_MOD,ev[i].data.fd,&event);
				//cout << "set socket epollin" <<endl;
            }*/
		}
	}

	for(vector<int>::iterator itr = sockets.begin(), last = sockets.end(); itr != last; itr++){
		close(*itr);
	}
	close(epfd);
	printf("clear socket....\n");
	return 0;
}

