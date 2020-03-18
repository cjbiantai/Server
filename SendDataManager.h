#pragma once
#include "Consts.h"

class SendDataManager
{
public:
    SendDataManager(){}
    ~SendDataManager(){}

private:
    char data[BUFF_SIZE + HEAD_LENGTH];
    char buff[BUFF_SIZE + HEAD_LENGTH];
};

