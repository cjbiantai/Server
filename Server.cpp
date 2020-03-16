#include "MySocket.h"
#include "Timer.h"

#include "Consts.h"

char data[BUFF_SIZE + HEAD_LENGTH],buff[BUFF_SIZE + HEAD_LENGTH];

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
    Timer timer;
    while(true){
        mySocket.Work(data, buff);
        timer.WaitForMSeconds(PER_FRAME_TIME);
        if(mySocket.GetClientNumber() == MAX_USERS)
            mySocket.Broad(buff);
    }
    return 0;
}

