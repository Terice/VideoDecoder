#ifndef PARSER_H__
#define PARSER_H__

#include "bitsbuf.h"
#include "SPS.h"
#include "PPS.h"


typedef struct PARAMETERSV
{
    uint32_t ChromaArrayType           ;
    uint32_t MaxFrameNum               ;
    
    uint32_t PicHeightInMbs;           ;
    uint32_t PicWidthInMbs             ;
    uint32_t PicHeightInMapUnits       ;
    uint32_t FrameHeightInMbs          ;
    uint32_t PicSizeInMapUnits         ;
    uint32_t PicSizeInMbs              ;


    uint32_t BitDepthY                 ;
    uint32_t BitDepthC                 ;
    uint32_t QpBdOffsetY               ;
    uint32_t QpBdOffsetC               ;

    uint32_t sliceGroups               ;
    uint32_t numRefIdx10DefaultActive  ;
    uint32_t numRefIdx11DefaultActive  ;

    uint32_t MbWidthInChroma           ;
    uint32_t MbHeightInChroma          ;

    uint8_t SubWidthC               = 1;
    uint8_t SubHeightC              = 1;
}ParametersV;
typedef struct PARAMETESS
{
    SPS_data* sps;
    PPS_data* pps;
}ParametersS;
class Debug;
class cabac;

class Parser
{
private:
    Bitsbuf* bitstream;
public:
    Debug* debug;
    cabac* cabac_core;
    ParametersV* get_pv();
    ParametersS* get_ps();
    ParametersV* pV;
    ParametersS* pS;
    int slice_idx;

    Bitsbuf* get_bitbuf();

    class NAL* cur_nal;
    bool find_nextNAL();
    bool read_bi();
    bool algi();
    bool read_al();
    uint64_t next(uint32_t size);

    uint64_t read_un(uchar size);
    int64_t  read_sn(uchar size);

    uint64_t read_ue();
    int64_t  read_se();
    uint64_t read_me(uint16_t ChromaArrayType, uint32_t mb_type);//映射指数
    uint64_t read_te(uint32_t range);//截断指数
    uint64_t read_ce();
    uint64_t read_ae(int);

    //从当前缓冲区中读入一个char，会强制对齐
    uchar read_ch();

    void rfsh();
    //以熵编码标志位做选择的两种读取方式
    uint16_t read_12();

    Parser();
    Parser(FILE* datares, Debug*);
    Parser(Bitsbuf* bitstream); //, cabac* cabac_core
    ~Parser();
};
#endif