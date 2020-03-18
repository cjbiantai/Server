#include "DataCenter.h"

DataCenter::DataCenter()
{
    data = (char*)malloc(sizeof(char) * (BUFF_SIZE + HEAD_LENGTH));
    buff = (char*)malloc(sizeof(char) * (BUFF_SIZE + HEAD_LENGTH));
}

DataCenter::~DataCenter()
{
    free(data);
    free(buff);
}

void DataCenter::AddHead(int symbol, int length, int lengthBase)
{
    buff[0] = symbol;
    buff[1] = length / lengthBase + 1;
    buff[2] = length % lengthBase + 1;
}
void DataCenter::ChangeDataAt(char c, int idx)
{
    data[idx] = c;
}
void DataCenter::ChangeBuffAt(char c, int idx)
{
    buff[idx] = c;
}

char* DataCenter::GetDataArray()
{
    return data;
}

char* DataCenter::GetBuffArray()
{
    return buff;
}

char DataCenter::GetDataCharAt(int idx)
{
    return data[idx];
}

char DataCenter::GetBuffCharAt(int idx)
{
    return buff[idx];
}

