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
    char popData();
    char getDataAt(int);
    RecvDataManager();
private:
    int front,length;
    char buff[BUFF_SIZE * 10];
};

