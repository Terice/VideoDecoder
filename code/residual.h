#ifndef RESIDUAL_H__
#define RESIDUAL_H__

#include <iostream>
#include "array2d.h"

//residual 类应该返回一个对象，包含了宏块的残差的所有数据
//1、宏块残差的类型：这里只有三种，分别是AC+DC     4x4     8x8
//然后还应该有一个表示coded_block_flag的标志，
//对于AC+DC 是1+16个，对于4x4 是16个， 对于8x8 是4个。
class residual
{
private:
    uint8_t codedPatternLuma;
    uint8_t codedPatternchroma;
    uint8_t cabacFlag;
    int8_t qP;
    bool  TransformBypassModeFlag;
    class Parser* parser;
    class macroblock* mb;
    class Decoder* decoder;
    class Debug* debug;

    void Decode_Intra4x4();
    void Decode_Intra16x16();
    void Decode_Chroma(int iCbCr);

    void Parser_Intra4x4(int index);
    void Parser_Intra8x8(int index);
    void Parser_Intra16x16();
    void Parser_Inter();
    void Parser_Chroma();
    //if luma of intra_16x16 or chroma residual 
    //mode 的意思：如果是16x16帧内预测或者c是色度的矩阵，那么这个值传1，否则不传
    class matrix& ScalingAndTransform_Residual4x4Blocks(int BitDepth, int qP, class matrix* c_ij, uint8_t mode = 0);
    //syntaxValue的值：16^2 作为一个十进制数指定当前用cat后缀的句法元素是哪种解析模式，一共12种（查表）
    //最后一位16^1 的低四位指当前的块索引
    void (residual::*residual_block)(class block* bl, int syntaxValue, uint8_t startIdx, uint8_t endIdx, uint8_t length);
                 void residual_block_cavlc(block* bl, int syntaxValue, uint8_t startIdx, uint8_t endIdx, uint8_t length);
                 void residual_block_cabac(block* bl, int syntaxValue, uint8_t startIdx, uint8_t endIdx, uint8_t length);
public:
    //残差块的解码模式：0是16x16 1 是4x4     2 是8x8
    uint8_t mode;// 0 intra1616        1   4x4            2 8x8

    //for luma，must do an [0] calc，and to 4x4 8x8 get block respectivly
    //对16x16 [0]取到DC系数block，[1]取到AC系数block
    block** luma;
    //[0]是AC block，[1]是DC block
    //第二级的[0]是Cb，[1]是Cr
    block** chroma;

    matrix* residual_Y;
    matrix* residual_U;
    matrix* residual_V;
    //根据指定的解析模式进行解析 数据就在这个对象的上面  
    void Parse(uint8_t residualParserMode);
    //decode
    void Decode(uint8_t mode);
    void ZeroAll();
    void Calc();
    //第一个是指定的宏块    第二个是解析器指针
    residual(class macroblock* ma, Parser* pa);
    ~residual();
};




#endif