#include "MySocket.h"

MySocket::MySocket(int port)
{
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
void MySocket::Work(char *data, char* buff) 
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
            clientDatas.ClientFDSet.insert(client_fd);
            if(clientDatas.clientData.find(client_fd) == clientDatas.clientData.end()) 
            {
                    clientDatas.clientData[client_fd] = RecvDataManager();
            }
            HandleRecvData(client_fd, data, buff);
        }
    }
}

void MySocket::HandleRecvData(int client_fd, char* data, char* buff)
{
    RecvDataManager& recvDataManager = clientDatas.clientData[client_fd];
    int ret = recv(client_fd, data, recvDataManager.emptySize(), 0);
    if(ret > 0) 
    {
        for(int i = 0;i < ret; ++i) 
        {
            recvDataManager.pushData(data[i]);  
        }
        HandleData(client_fd, data, buff);
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

void MySocket::HandleData(int client_fd, char* data, char* buff)
{
    RecvDataManager& RecvDataManager = clientDatas.clientData[client_fd];
    while(RecvDataManager.size() >= HEAD_LENGTH) 
    {
        int symbol = RecvDataManager.getDataAt(0);
        int package_length = (int)(RecvDataManager.getDataAt(1) - 1 ) * LENGTH_BASE + (int)RecvDataManager.getDataAt(2) - 1;
        if(RecvDataManager.size() >= package_length + HEAD_LENGTH) 
        {
            for(int i = 0; i < HEAD_LENGTH; ++i) RecvDataManager.popData();
            for(int i = 0; i < package_length; ++i) 
            {
                data[i] = RecvDataManager.popData();
            }
            HandleMsg(client_fd, symbol, package_length, data, buff);        
        }else 
        {
            break;
        }
    }
}

void MySocket::HandleMsg(int client_fd, int symbol, int package_length, char* data, char* buff)
{
    if(symbol == FRAME_DATA)
    {
        PlayerInput playerInput;    
        int ret = playerInput.ParseFromArray(data, package_length);
        if(ret == -1)
        {
            printf("PlayerInput ParseFromArray Error, Error Message is : \n");
            for(int i = 0; i < package_length; ++i)
            {
                printf("%03d ", data[i]);
            }
            printf("\n");
            return ;
        }
        FrameData fmData;
        PlayerInput* ptr = fmData.mutable_playerinput();
        ptr -> CopyFrom(playerInput);
        if(ptr -> type() != 0)  clientDatas.ClientFrameData[client_fd] = fmData;
    }else if(symbol == LOG_IN || symbol == SIGN_UP)
    {
        HandleLogIn(symbol, package_length, client_fd, data, buff);
    }
}

void MySocket::SendToClient(int client_fd, int length, char* buff)
{
    int ret = send(client_fd, buff, length, 0);
    if(ret == -1)
    {
        printf("send to client error: client_fd = %d, errno = %d, (%s)\n", client_fd, errno, strerror(errno));
    }else 
    {
        printf("send to client :client_fd = %d,lenth = %d\n", client_fd, length);
    }
}


void MySocket::SendToAllClients(int length, char* buff)
{
    std::set<int>::iterator it;
    for(it = clientDatas.ClientFDSet.begin(); it != clientDatas.ClientFDSet.end(); it ++ )
    {
        SendToClient((*it), length, buff);
    }
}

void MySocket::Broad(char* buff)
{
    clientDatas.frameNo += 1;
    std::map<int, FrameData>::iterator it;
    for(it = clientDatas.ClientFrameData.begin(); it!= clientDatas.ClientFrameData.end(); it++)
    {
        FrameData fmData = (*it).second;
        fmData.set_frameno(clientDatas.frameNo);
        int length = fmData.ByteSize();
        if(!fmData.SerializeToArray(buff + HEAD_LENGTH, BUFF_SIZE))
        {
            printf("SerializeToArray Error: error at framNo = %d, client_fd = %d\n", clientDatas.frameNo, (*it).first);
            continue;
        }
        PlayerInput *playerInput = fmData.mutable_playerinput();
        clientDatas.AllFrameData[clientDatas.totalFrame++] = fmData;
        buff[0] = FRAME_DATA;
        buff[1] = length / LENGTH_BASE + 1;
        buff[2] = length % LENGTH_BASE + 1;
        SendToAllClients(length + HEAD_LENGTH, buff);
    }
}
void MySocket::HandleLogIn(int symbol, int length, int client_fd, char* data, char* buff)
{
    ResponseInfo resp;
    UserInfo user;
    int ret;
    char query_string[255];
    resp.set_yourfd(client_fd);
    if(!user.ParseFromArray(data, length))
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
                        clientDatas.Users.insert(user.user_name());
                        clientDatas.FdToUser[client_fd] = user.user_name();
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
    if(!resp.SerializeToArray(buff + HEAD_LENGTH, BUFF_SIZE))
    {
        printf("SerializeToArray Error\n");
        return ;
    }
    length = resp.ByteSize();
    buff[0] = RESPONSE;
    buff[1] = length / LENGTH_BASE + 1;
    buff[2] = length % LENGTH_BASE + 1;
    SendToClient(client_fd, length + HEAD_LENGTH, buff);
    if(symbol == LOG_IN && resp.message() == "OK") 
    {
        SendToClientHistoryFrame(client_fd, buff);
    }
}

void MySocket::SendToClientHistoryFrame(int client_fd, char* buff)
{
    for(int i = 0; i < clientDatas.totalFrame; i++)
    {
        FrameData fmData = clientDatas.AllFrameData[i];
        int length = fmData.ByteSize();
        if(!fmData.SerializeToArray(buff + HEAD_LENGTH, BUFF_SIZE))
        {
            printf("SerializeToArray Error: error at framNo = %d, client_fd = %d\n", fmData.frameno(), client_fd);
            continue;
        }
        buff[0] = FRAME_DATA;
        buff[1] = length / LENGTH_BASE + 1;
        buff[2] = length % LENGTH_BASE + 1;
        SendToClient(client_fd, length + HEAD_LENGTH, buff);

    }
}

