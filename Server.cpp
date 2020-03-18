#include "MySocket.h"
#include "Timer.h"

#include "Consts.h"

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
        mySocket.Work();
        if(!timer.WaitForMSeconds(PER_FRAME_TIME)) continue;
        mySocket.Broad();
    }
    return 0;
}

