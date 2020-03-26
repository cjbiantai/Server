#include "MySocket.h"
#include "Timer.h"

#include "Consts.h"

int main(int argc, char** argv)
{
    int oneRoomNumber;
    if (argc < 2)
    {
        oneRoomNumber = 1;
    }else oneRoomNumber = atoi(argv[1]);

    MySocket mySocket(DEFAULT_PORT);
    mySocket.Init();
    mySocket.InitEpoll();
    Timer timer;
    while(true){
        mySocket.Work();
        if(!timer.WaitForMSeconds(PER_FRAME_TIME)) continue;
        if(mySocket.GetClientNumber() < oneRoomNumber) continue;
        mySocket.Broad();
    }
    return 0;
}

