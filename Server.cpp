#include <sys/time.h>

#include "RecvDataManager.h"
#include "MySocket.h"

using namespace std;


char data[BUFF_SIZE + HEAD_LENGTH],buff[BUFF_SIZE + HEAD_LENGTH];
long timeStamp;
struct timeval tv;

long GetTimeNow()
{
    int ret = gettimeofday(&tv,NULL);
    if(ret == -1) 
    {
        printf("GetTimeNow Error: errno = %d, (%s)\n", errno, strerror(errno));
        return -1;
    }
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

int main(int argc, char** argv)
{
    
    int port;
    if (argc < 2)
    {
        port = DEFAULT_PORT;
    }else port = atoi(argv[1]);
    MySocket mySocket(port);
    mySocket.Init();
    mySocket.InitEpoll();
    while(true){
        mySocket.Work(data, buff);
        if(GetTimeNow() - timeStamp < 20000) {}
        timeStamp = GetTimeNow();
        mySocket.Broad(buff);
    }
    return 0;
}

