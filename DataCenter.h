#pragma once
#include "Consts.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

class DataCenter
{
public:
    DataCenter();
    ~DataCenter();
    void AddHead(int, int, int); 
    void ChangeDataAt(char, int);
    void ChangeBuffAt(char, int);
    char* GetDataArray();
    char* GetBuffArray();
    char GetDataCharAt(int);
    char GetBuffCharAt(int);
private:
    char *data, *buff;
};

