#include "MySocket.h"

MySocket::MySocket(int port)
{
    totalUser = 0;
    this->port = port;
}

MySocket::MySocket()
{
    MySocket(DEFAULT_PORT);
}
MySocket::~MySocket()
{
    free(events);
}
void MySocket::Init()
{
   server_fd = socket(AF_INET, SOCK_STREAM, 0);
   if(server_fd == -1) 
   {
        printf("create socket error, errno = %d, (%s)\n", errno, strerror(errno));
        exit(-1);
   }
   memset(&server_addr, 0, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   server_addr.sin_port = htons(port);

   int value = 1;
   setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&value, sizeof(value));

   int bind_ret = bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
   if(bind_ret == -1) 
   {
        printf("bind error: errno = %d, (%s)\n", errno, strerror(errno));
        exit(-1);
   }else 
   {
       printf("bind [0.0.0.0:%d] ok!\n", port);
   }

   int listen_ret = listen(server_fd, BACKLOG);
   if(listen_ret == -1) 
   {
       printf("listen error:errno = %d, (%s)\n", errno, strerror(errno));
       exit(-1);
   }else 
   {
       printf("start listening on socket fd [%d] ... \n", server_fd);
   }
}

void MySocket::InitEpoll()
{
    epoll_fd = epoll_create(1);
    ev.data.fd = server_fd;
    ev.events = EPOLLIN;

    int ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);
    if(ret == -1) 
    {
        printf("epoll_ctl error: errno = %d, (%s)\n", errno, strerror(errno));
        exit(-1);
    }
    events = (struct epoll_event*)malloc(sizeof(ev)*MAX_EVENTS);
}
void MySocket::Work() 
{
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1);
    for(int i = 0; i < nfds; ++i)
    {
        int fd = events[i].data.fd;
        int fd_events = events[i].events;
        if((fd_events & EPOLLERR) || (fd_events & EPOLLHUP) || (!(fd_events & EPOLLIN)))
        {
            printf("fd:%d error \n", fd);
            clientDatas.clientData.erase(fd);
            clientDatas.ClientFDSet.erase(fd);
            clientDatas.ClientFrameData.erase(fd);
            if(clientDatas.FdToUser.find(fd) != clientDatas.FdToUser.end())
            {
                clientDatas.Users.erase(clientDatas.FdToUser[fd]);
                clientDatas.FdToUser.erase(fd);
            }
            close(fd);
            continue;
        }
        if(events[i].data.fd == server_fd)
        {
            int client_fd = accept(server_fd, (struct sockaddr*)NULL, NULL);
            if(client_fd == -1)
            {
                printf("accpet socket error: errno = %d, (%s)\n", errno,strerror(errno));
            }
            ev.data.fd = client_fd;
            ev.events = EPOLLIN;
            int epoll_ret = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
            if(epoll_ret == -1)
            {
                printf("epoll_ctl error: errno = %d, (%s)\n", errno, strerror(errno));
                continue;
            }
        }else 
        {
            int ret = -1,client_fd = events[i].data.fd;
            if(clientDatas.clientData.find(client_fd) == clientDatas.clientData.end()) 
            {
                clientDatas.clientData[client_fd] = RecvDataManager();
            }
            HandleRecvData(client_fd);
        }
    }
}

void MySocket::HandleRecvData(int client_fd)
{
    RecvDataManager& recvDataManager = clientDatas.clientData[client_fd];
    int ret = recv(client_fd, dataCenter.GetDataArray(), recvDataManager.emptySize(), 0);
    if(ret > 0) 
    {
        for(int i = 0;i < ret; ++i) 
        {
            recvDataManager.pushData(dataCenter.GetDataCharAt(i));  
        }
        HandleData(client_fd);
    }else if(ret < 0)
    {
        printf("recv data from fd:%d error, errno = %d, (%s)\n", client_fd, errno, strerror(errno));
        if(errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
        {   
            clientDatas.clientData.erase(client_fd);
            clientDatas.ClientFDSet.erase(client_fd);
            clientDatas.ClientFrameData.erase(client_fd);
            if(clientDatas.FdToUser.find(client_fd) != clientDatas.FdToUser.end())
            {
                clientDatas.Users.erase(clientDatas.FdToUser[client_fd]);
                clientDatas.FdToUser.erase(client_fd);
            }
            close(client_fd);

        }
    }else 
    {
        printf("socket:%d closed\n", client_fd);
        clientDatas.clientData.erase(client_fd);
        clientDatas.ClientFDSet.erase(client_fd);
        clientDatas.ClientFrameData.erase(client_fd);
        if(clientDatas.FdToUser.find(client_fd) != clientDatas.FdToUser.end())
        {
            clientDatas.Users.erase(clientDatas.FdToUser[client_fd]);
            clientDatas.FdToUser.erase(client_fd);
        }
        int close_ret = close(client_fd);
        if(close_ret == -1) 
        {
            printf("close socket error: errno = %d, (%s)\n", errno, strerror(errno));
        }
    }
}

void MySocket::HandleData(int client_fd)
{
    RecvDataManager& RecvDataManager = clientDatas.clientData[client_fd];
    while(RecvDataManager.size() >= HEAD_LENGTH) 
    {
        int symbol = RecvDataManager.GetSymbol();
        int package_length = RecvDataManager.GetPackageLength();
        if(RecvDataManager.size() >= package_length + HEAD_LENGTH) 
        {
            for(int i = 0; i < HEAD_LENGTH; ++i) RecvDataManager.popData();
            for(int i = 0; i < package_length; ++i) 
            {
                dataCenter.ChangeDataAt(RecvDataManager.popData(), i);
            }
            HandleMsg(client_fd, symbol, package_length);        
        }else 
        {
            break;
        }
    }
}

void MySocket::HandleMsg(int client_fd, int symbol, int package_length)
{
    if(symbol == FRAME_DATA)
    {
        PlayerInput playerInput;    
        int ret = playerInput.ParseFromArray(dataCenter.GetDataArray(), package_length);
        if(ret == -1)
        {
            printf("PlayerInput ParseFromArray Error, Error Message is : \n");
            return ;
        }
        FrameData fmData;
        PlayerInput* ptr = fmData.mutable_playerinput();
        ptr -> CopyFrom(playerInput);
        clientDatas.ClientFrameData[client_fd] = fmData;
    }else if(symbol == LOG_IN || symbol == SIGN_UP)
    {
        HandleLogIn(symbol, package_length, client_fd);
    }else if(symbol == REQUEST_FRAMEDATAS)
    {
        FrameData fmData;
        int ret = fmData.ParseFromArray(dataCenter.GetDataArray(), package_length);
        if(ret == -1) 
        {
            printf("FrameData ParseFromArray Error, Errror message is: \n");
            for(int i = 0; i < package_length; ++i)
            {
                printf("%03d ", dataCenter.GetDataCharAt(i));
            }
            printf("\n");
            return ;
        }
        SendToClientHistoryFrame(client_fd, fmData.frameno());
    }
}

void MySocket::SendToClient(int client_fd, int length)
{
    int ret = send(client_fd, dataCenter.GetBuffArray(), length, 0);
    if(ret == -1)
    {
        printf("send to client error: client_fd = %d, errno = %d, (%s)\n", client_fd, errno, strerror(errno));
        clientDatas.clientData.erase(client_fd);
        clientDatas.ClientFDSet.erase(client_fd);
        clientDatas.ClientFrameData.erase(client_fd);
        if(clientDatas.FdToUser.find(client_fd) != clientDatas.FdToUser.end())
        {
            clientDatas.Users.erase(clientDatas.FdToUser[client_fd]);
            clientDatas.FdToUser.erase(client_fd);
        }
        close(client_fd);

    }
}


void MySocket::SendToAllClients(int length)
{
    std::set<int>::iterator it;
    for(it = clientDatas.ClientFDSet.begin(); it != clientDatas.ClientFDSet.end(); it ++ )
    {
        SendToClient((*it), length);
    }
}

void MySocket::Broad()
{
    clientDatas.frameNo += 1;
    clientDatas.HistroyFrameNoToIdx[clientDatas.frameNo] = clientDatas.totalFrame;
    std::map<int, FrameData>::iterator it;
    for(it = clientDatas.ClientFrameData.begin(); it!= clientDatas.ClientFrameData.end(); it++)
    {
        FrameData fmData = (*it).second;
        PlayerInput* ptr = fmData.mutable_playerinput();
        ptr ->set_characterid(clientDatas.UserId[ptr -> playername()]);
        fmData.set_frameno(clientDatas.frameNo);
        if(!fmData.SerializeToArray(dataCenter.GetBuffArray() + HEAD_LENGTH, BUFF_SIZE))
        {
            printf("SerializeToArray Error: error at framNo = %d, client_fd = %d\n", clientDatas.frameNo, (*it).first);
            continue;
        }
        dataCenter.AddHead(FRAME_DATA, fmData.ByteSize(), LENGTH_BASE);
        clientDatas.AllFrameData[clientDatas.totalFrame++] = fmData;
        SendToAllClients(fmData.ByteSize() + HEAD_LENGTH);
        ptr -> set_type(0);
        (*it).second = fmData;
    }
}
void MySocket::HandleLogIn(int symbol, int length, int client_fd)
{
    ResponseInfo resp;
    UserInfo user;
    int ret;
    char query_string[255];
    resp.set_yourfd(client_fd);
    for(int i = 0; i < length; ++i)
    {
        printf("%3d ", dataCenter.GetDataCharAt(i));
    }
    printf("\n");
    if(!user.ParseFromArray(dataCenter.GetDataArray(), length))
    {
        printf("ParseFromArray Error\n");
        resp.set_response_id(-1);
        resp.set_message("UNKNOW_REQUEST_DATA_TYPE");
    }
    else if(LOG_IN == symbol) 
    {
        sprintf(query_string, "select * from %s where name='%s'", TABLENAME, user.user_name().c_str());
        ret = query_sql(query_string);
        resp.set_response_id(ret);
        switch (ret) {
            case CONNECT_TO_SQL_ERROR:
                resp.set_message("INTERNAL_ERROR");
                break;
            case QUERY_SQL_ERROR:
                resp.set_message("INTERNAL_ERROR");
                break;
            case QUERY_EMPTY:
                resp.set_message("USER DOES NOT EXIST");
                break;
            case QUERY_OK:
                sprintf(query_string, "select * from %s where name = '%s' and password = '%s'", TABLENAME, user.user_name().c_str(), user.user_password().c_str());
                ret = query_sql(query_string);
                if(ret == QUERY_OK){
                    if(clientDatas.Users.find(user.user_name()) == clientDatas.Users.end())
                    {

                        clientDatas.ClientFDSet.insert(client_fd);
                        clientDatas.Users.insert(user.user_name());
                        clientDatas.FdToUser[client_fd] = user.user_name();
                        clientDatas.UserId[user.user_name()] = totalUser++; 
                        resp.set_message("OK");
                    }else 
                    {
                        resp.set_message("user has already log in\n");
                        resp.set_response_id(RE_LOG_IN);
                    }
                }else
                {
                    resp.set_message("PASSWORD WRONG");
                    resp.set_response_id(QUERY_EMPTY);
                }
                break;
        }
    }else
    {
        sprintf(query_string, "select * from %s where name='%s'", TABLENAME, user.user_name().c_str());
        ret = query_sql(query_string);
        resp.set_response_id(ret + SIGN_UP_OFFSET);
        switch (ret)
        {
            case CONNECT_TO_SQL_ERROR:
                resp.set_message("INTERNAL_ERROR");
                break;
            case QUERY_SQL_ERROR:
                resp.set_message("INTERNAL_ERROR");
                break;
            case QUERY_OK:
                resp.set_message("USER HAS EXISTED");
                resp.set_response_id(QUERY_EMPTY + SIGN_UP_OFFSET);
                break;
            default:
                sprintf(query_string, "insert into %s value('%s','%s')", TABLENAME, user.user_name().c_str(), user.user_password().c_str());
                ret = query_sql(query_string);
                resp.set_response_id(QUERY_OK + SIGN_UP_OFFSET);
                resp.set_message("OK");
        }
    }    
    if(!resp.SerializeToArray(dataCenter.GetBuffArray() + HEAD_LENGTH, BUFF_SIZE))
    {
        printf("SerializeToArray Error\n");
        return ;
    }
    dataCenter.AddHead(RESPONSE, resp.ByteSize(), LENGTH_BASE);
    SendToClient(client_fd, resp.ByteSize() + HEAD_LENGTH);
    /*
    if(resp.response_id() == 0)
    {
        printf(" ------------------------>_<----------------------------\n");
        SendToClientHistoryFrame(client_fd, 0);    
    }
    */
}

void MySocket::SendToClientHistoryFrame(int client_fd, int reqFrameNo)
{
    int idx = clientDatas.HistroyFrameNoToIdx[reqFrameNo];
    for(int i = idx; i < clientDatas.totalFrame; i++)
    {
        FrameData fmData = clientDatas.AllFrameData[i];
        if(!fmData.SerializeToArray(dataCenter.GetBuffArray() + HEAD_LENGTH, BUFF_SIZE))
        {
            printf("SerializeToArray Error: error at framNo = %d, client_fd = %d\n", fmData.frameno(), client_fd);
            continue;
        }
        dataCenter.AddHead(FRAME_DATA, fmData.ByteSize(), LENGTH_BASE);
        printf("send to client: %d, frameno: %d\n", client_fd, fmData.frameno());
        SendToClient(client_fd, fmData.ByteSize() + HEAD_LENGTH);
    }
}


int MySocket::GetClientNumber()
{
    return clientDatas.ClientFDSet.size();
}
