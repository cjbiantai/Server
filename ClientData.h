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

    //储存每个帧号对应的最早的帧信息，用于追帧
    int HistroyFrameNoToIdx[MAX_FRAMEDATAS];

    //Fd 到 用户名 的映射
    std::map<int, std::string>FdToUser;
    //已登录用户名集合 
    std::set<std::string>Users;

    //储存每个用户登录时的序号
    std::map<std::string, int>UserId;
};


