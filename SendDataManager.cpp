#include "SendDataManager.h"

int SendDataManager::AddData(int length, char* data)
{
    if(lengthq.size() >= SEND_BUFF) 
    {
        printf("add to send queue error: the queue is full\n");
        return -1;
    }
}
