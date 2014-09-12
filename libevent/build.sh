#!/bin/sh

LIB_EVENT_DIR="/home/users/yangjun03/libevent/libevent"
LIB_EVENT_INCLUDE=${LIB_EVENT_DIR}"/installdir/include"
LIB_EVENT_LIB=${LIB_EVENT_DIR}"/installdir/lib"
CXXFLAGS="-pipe -W -Wall -fPIC -D_GNU_SOURCE -D__STDC_LIMIT_MACROS"
LDFLAGS="-lpthread -lrt -lz -levent -levent_pthreads"

export LD_LIBRARY_PATH=${LIB_EVENT_LIB}:${LD_LIBRARY_PATH}

g++ -I. -I$LIB_EVENT_DIR -I$LIB_EVENT_INCLUDE -L$LIB_EVENT_LIB  $CXXFLAGS $LDFLAGS -o echo_client echo_client.cpp
#g++ -I. -I$LIB_EVENT_DIR -I$LIB_EVENT_INCLUDE -L$LIB_EVENT_LIB  $CXXFLAGS $LDFLAGS -o event1 event1.cpp
#g++ -I. -I$LIB_EVENT_DIR -I$LIB_EVENT_INCLUDE -L$LIB_EVENT_LIB  $CXXFLAGS $LDFLAGS -o echo echo.c
#g++ -I. -I$LIB_EVENT_DIR -I$LIB_EVENT_INCLUDE -L$LIB_EVENT_LIB  $CXXFLAGS $LDFLAGS -o hello hello.cpp
#g++ -I. -I$LIB_EVENT_DIR -I$LIB_EVENT_INCLUDE -L$LIB_EVENT_LIB  $CXXFLAGS $LDFLAGS -o client client.cpp
#g++ -I. -I$LIB_EVENT_DIR -I$LIB_EVENT_INCLUDE -L$LIB_EVENT_LIB  $CXXFLAGS $LDFLAGS -o server server.cpp
