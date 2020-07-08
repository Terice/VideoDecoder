

#include <stdio.h>
#include <string>
#include <iostream>
using namespace std;

#include "Parser.h"
#include "Decoder.h"
#include "NAL.h"
#include "slice.h"
#define FILEPATH "../../resource/fox (copy).264"
#include "Debug.h"

int main()
{
    FILE* fp;
    
    if((fp = fopen(FILEPATH, "r")) == NULL) exit(-1);
    fseek(fp, 0x2FFL,SEEK_SET);

    Debug debug("./debug_config");
    Parser parser(fp, &debug);
    Decoder decoder;
    
    //SPS
    NAL nal(&parser, &decoder);
    nal.decode();
    
    //PPS
    fseek(fp, 0x2E0L,SEEK_SET);
    parser.rfsh();
    NAL nal2(&parser, &decoder);
    nal2.decode();

    //the second IDR
    //pic 201
    // debug.set_control_all(true);
    // fseek(fp, 0x9C18DL,SEEK_SET);
    // parser.rfsh();
    // NAL nal10(&parser, &decoder);
    // nal10.decode();

    // fseek(fp, 0x9C18AL,SEEK_SET);
    // debug.set_control_all(false);
    
    while(parser.find_nextNAL())
    {
        NAL nal(&parser, &decoder);
        nal.decode();
    }
    

    fclose(fp);
    return 0;
}