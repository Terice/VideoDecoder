#ifndef NAL_H__
#define NAL_H__

#include <iostream>

enum NALtype
{
    Undef       = 0,
    Non_IDR     = 1,
    SliceA      = 2,
    SliceB      = 3,
    SliceC      = 4,
    IDR         = 5,
    SEI         = 6,
    SPS         = 7,
    PPS         = 8,
};
class Parser;
class Decoder;
class picture;


typedef unsigned char uchar;
class NAL
{
private:

    Parser* parser;

    void decode_PPS();
    void decode_SPS();
    uchar** decode_PIC();
    void* data;

    bool is_B;
public: 
    Decoder* decoder;
    picture* pic;
    NALtype type;
    uint16_t nal_unit_type       ;
    uint16_t forbidden_zero_bit  ;
    uint16_t nal_ref_idc         ;
    const void* getdata();
    
    bool decode();
    NAL();
    NAL(Parser* parser, Decoder* decoder);
    ~NAL();
};


#endif