#include "MySocket.h"

MySocket::MySocket(int port)
{
    frameNo = 0;
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
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    for(int i = 0; i < nfds; ++i)
    {
        int fd = events[i].data.fd;
        int fd_events = events[i].events;
        if((fd_events & EPOLLERR) || (fd_events & EPOLLHUP) || (!(fd_events & EPOLLIN)))
        {
            printf("fd:%d error \n", fd);
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
            if(ClientData.find(client_fd) == ClientData.end()) 
            {
                    ClientData[client_fd] = RecvDataManager();
            }
            HandleRecvData(client_fd, data, buff);
        }
    }
}

void MySocket::HandleRecvData(int client_fd, char* data, char* buff)
{
    RecvDataManager& recvDataManager = ClientData[client_fd];
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
    }else 
    {
        printf("socket:%d closed\n", client_fd);
        ClientData.erase(client_fd);
        ClientFDSet.erase(client_fd);
        int close_ret = close(client_fd);
        if(close_ret == -1) 
        {
            printf("close socket error: errno = %d, (%s)\n", errno, strerror(errno));
        }
    }
}

void MySocket::HandleData(int client_fd, char* data, char* buff)
{
    RecvDataManager& RecvDataManager = ClientData[client_fd];
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
        ClientFrameData[client_fd] = fmData;
    }else if(symbol == LOG_IN)
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
        printf("send to client :client_fd = %d,lenth = %d, data = %s\n", client_fd, length, buff);
    }
}


void MySocket::SendToAllClients(int length, char* buff)
{
    std::set<int>::iterator it;
    for(it = ClientFDSet.begin(); it != ClientFDSet.end(); it ++ )
    {
        SendToClient((*it), length, buff);
    }
}

void MySocket::Broad(char* buff)
{
    frameNo += 1;
    std::map<int, FrameData>::iterator it;
    for(it = ClientFrameData.begin(); it!= ClientFrameData.end(); it++)
    {
        FrameData fmData = (*it).second;
        fmData.set_frameno(frameNo);
        int length = fmData.ByteSize();
        if(!fmData.SerializeToArray(buff, BUFF_SIZE + HEAD_LENGTH))
        {
            printf("SerializeToArray Error: error at framNo = %d, client_fd = %d\n", frameNo, (*it).first);
            continue;
        }
        AllFrameData.push_back(fmData);
        SendToClient((*it).first, length, buff);
    }
}
void MySocket::HandleLogIn(int symbol, int length, int client_fd, char* data, char* buff)
{
    ResponseInfo resp;
    UserInfo user;
    char query_string[255];
    int ret;
    if(!user.ParseFromArray(data, length))
    {
        printf("ParseFromArray Error\n");
        resp.set_response_id(-1);
        resp.set_message("UNKNOW_REQUEST_DATA_TYPE");
    }
    else if(LOG_IN == symbol) 
    {
        sprintf(query_string, "select * from %s where name='%s'", TABLENAME, user.user_name().c_str());
        int ret = query_sql(query_string);
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
                    resp.set_message("OK");
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
        int ret = query_sql(query_string);
        resp.set_response_id(ret);
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
                resp.set_response_id(QUERY_EMPTY);
                break;
            default:
                sprintf(query_string, "insert into %s value('%s','%s')", TABLENAME, user.user_name().c_str(), user.user_password().c_str());
                ret = query_sql(query_string);
                resp.set_response_id(QUERY_OK);
                resp.set_message("OK");
        }
    }    
    if(!resp.SerializeToArray(buff + HEAD_LENGTH, BUFF_SIZE + HEAD_LENGTH))
    {
        printf("SerializeToArray Error\n");
        return ;
    }
    length = resp.ByteSize();
    buff[0] = symbol;
    buff[1] = length / LENGTH_BASE + 1;
    buff[2] = length % LENGTH_BASE + 1;
    SendToClient(client_fd, length + HEAD_LENGTH, buff);
}
