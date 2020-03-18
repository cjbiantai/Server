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

int RecvDataManager::GetSymbol()
{
    return buff[front];
}

int RecvDataManager::GetPackageLength()
{
    return (int)(buff[(front + 1) % BUFF_SIZE] -1) * LENGTH_BASE + buff[(front + 2) % BUFF_SIZE] - 1; 
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

RecvDataManager::RecvDataManager()
{
    front = 0;
    length = 0;
}


void RecvDataManager::Log()
{
    for(int i = 0; i < length; ++i)
    {
        printf("%3d ", buff[(front + i) % BUFF_SIZE]);
    }
    printf("\n");
}
