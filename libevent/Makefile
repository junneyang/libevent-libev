#This is the makefile of EpollTest

.PHONY:all
all:epoll_server epoll_client
epoll_server:
	g++ epoll_server.cpp -o epoll_server -lpthread
epoll_client:
	g++ epoll_client.cpp -o epoll_client -lpthread
clean:
	rm -f epoll_server epoll_client
	
