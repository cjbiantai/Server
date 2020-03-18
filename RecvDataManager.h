#pragma once
#include "Consts.h"
#include <stdio.h>

class RecvDataManager
{
public:
    bool isFull();
    int emptySize();
    int size();
    int pushData(char);
    int GetSymbol();
    int GetPackageLength();
    char popData();
    RecvDataManager();
    void Log();
private:
    int front,length;
    char buff[BUFF_SIZE * 10];
};

