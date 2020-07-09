

#include <stdio.h>
#include <string>
#include <iostream>
using namespace std;

#include "Parser.h"
#include "Decoder.h"
#include "Debug.h"
#include "NAL.h"

#define FILEPATH "../../../../resource/fox (copy).264"

int main()
{
    FILE* fp;
    
    if((fp = fopen(FILEPATH, "r")) == NULL) exit(-1);
    // fseek(fp, 0x2FFL,SEEK_SET);

    //这三个对象分别是Debug器，解析器，解码器
    Debug debug("./debug_config");
    Parser parser(fp, &debug);
    Decoder decoder;

    debug.de_DltTime("start");
    int i = 0;
    //如果找到了下一个NAL，那么就解码他，否则退出循环
    while(parser.find_nextNAL())
    {
        NAL nal(&parser, &decoder);
        nal.decode();

        cout << ">>frame:" << i << endl;
        debug.de_DltTime("");
        i++;
    }

    fclose(fp);
    return 0;
}