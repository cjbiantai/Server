#pragma once
#include <map>
#include <set>
#include "Consts.h"
#include "RecvDataManager.h"
#include "protobufs.pb.h"


class ClientData
{
public:
    ClientData()
    {
        frameNo = 0;
        totalFrame = 0;
    }
    //全部客户端集合
    std::set<int>ClientFDSet;
    //每个客户端的消息队列
    std::map<int, RecvDataManager>clientData;
    //每个客户端的帧消息
    std::map<int, FrameData>ClientFrameData;
    //记录所有帧信息，处理断线重连
    FrameData AllFrameData[MAX_FRAMEDATAS];
    //当前帧号和总帧数
    int frameNo,totalFrame;

    std::map<int, std::string>FdToUser;

    std::set<std::string>Users;
};


