

#include <stdio.h>
#include <string>

#include <iostream>
using namespace std;

#include "Parser.h"
#include "Decoder.h"
#include "Debug.h"
#include "NAL.h"

#define FILEPATH "../resource/fox.264"
#define FILEPATH_DEBUG_CONF "./Debug_conf.lua"

int main(int argc, char* argv[])
{
    FILE* fp;
    
    if((fp = fopen(FILEPATH, "r")) == NULL) {printf("input file err\n");exit(-1);};


    //这三个对象分别是Debug器，解析器，解码器
    Debug debug(FILEPATH_DEBUG_CONF);//Debug从配置的lua文件中读入控制变量
    Parser parser(fp, &debug);//parser从fp中读入字节数据，并且带上debug对象
    Decoder decoder;//Decoder用来做图像管理等，在nal运行的时候用到

    debug.set_TimeFlag();
    int i = 0;
    //如果找到了下一个NAL，那么就解码他，否则退出循环
    while(parser.find_nextNAL())
    {
        NAL nal(&parser, &decoder);
        nal.decode();
        i++;
    }

    fclose(fp);
    return 0;
}