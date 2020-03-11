#pragma once
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <set>
#include <map>
#include <list>

#include "Consts.h"
#include "RecvDataManager.h"
#include "protobufs.pb.h"
#include "Mmysql.h"

class MySocket
{
public:
    MySocket(int );
    MySocket();
    ~MySocket();
    void Init();
    void InitEpoll();
    void Work(char*, char*);
    void HandleRecvData(int, char*, char*);
    void HandleData(int, char*, char*);
    void HandleMsg(int, int, int, char*, char*);
    void SendToClient(int, int, char*);
    void SendToAllClients(int, char*);
    void Broad(char*);
    void HandleLogIn(int, int, int, char*, char*);


private:
    int port,server_fd;
    struct sockaddr_in server_addr;
    int epoll_fd;
    int frameNo;
    struct epoll_event ev;
    struct epoll_event* events;
    std::set<int>ClientFDSet;
    std::map<int, RecvDataManager>ClientData;
    std::map<int, FrameData>ClientFrameData;
    std::list<FrameData>AllFrameData;
};

