#include "RecvDataManager.h"

int RecvDataManager::emptySize()
{
    return (int)(BUFF_SIZE - length);
}

int RecvDataManager::size()
{
    return length;
}

bool RecvDataManager::isFull()
{
    return emptySize() == 0;
}

int RecvDataManager::pushData(char c)
{
    if(isFull())
    {
        printf("Error : This buffer is full\n");
        return -1;
    }
    buff[(front + length) % BUFF_SIZE] = c;
    length = length + 1;
    return 0;
}

char RecvDataManager::popData()
{
    if(emptySize() == BUFF_SIZE)
    {
        printf("Error : This buffer is empty\n");
        return -1;
    }
    char c = buff[front];
    front = (front + 1) % BUFF_SIZE;
    length -= 1;
    return c;
}

char RecvDataManager::getDataAt(int idx)
{
    if(size() <= idx || idx < 0) 
    {
        printf("index out of range\n");
        return -1;
    }
    return buff[(front + idx) % BUFF_SIZE];
}
RecvDataManager::RecvDataManager()
{
    front = 0;
    length = 0;
}


