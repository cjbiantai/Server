#pragma once
#include "Consts.h"
#include <queue>
#include <stdio.h>
#include <string.h>

class SendDataManager
{
public:
    SendDataManager() {}
    int AddData(int, char*);

private:
    std::queue<int> lengthq;
    char data[BUFF_SIZE * SEND_BUFF];
};

