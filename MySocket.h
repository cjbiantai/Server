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
#include "ClientData.h"

class MySocket
{
public:
    MySocket(int);
    MySocket();
    ~MySocket();
    //初始化Socket连接
    void Init();
    //初始化Epoll
    void InitEpoll();
    //监听消息 
    void Work(char*, char*);
    //收到消息时的消息处理 处理沾包
    void HandleRecvData(int, char*, char*);
    //处理消息队列 消息分包处理
    void HandleData(int, char*, char*);
    //分类型处理接受的消息
    void HandleMsg(int, int, int, char*, char*);
    //发送消息给单个客户端
    void SendToClient(int, int, char*);
    //发送消息给全体客户端
    void SendToAllClients(int, char*);
    //帧同步 广播消息
    void Broad(char*);
    //处理登录注册操作
    void HandleLogIn(int, int, int, char*, char*);

    void SendToClientHistoryFrame(int, char*);


private:
    //Socket和Epoll相关数据
    int port,server_fd;
    struct sockaddr_in server_addr;
    int epoll_fd;
    struct epoll_event ev;
    struct epoll_event* events;
    ClientData clientDatas;
};

