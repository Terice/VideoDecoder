#ifndef PICTURE_H__
#define PICTURE_H__

#include <iostream>
#include "array2d.h"
#include "enums.h"
#include "functions.h"

class macroblock;

class picture
{
private:
    uint height_mb;
    uint width_mb;
    bool type;
    //宏块数组
    array2d<macroblock*>* mb_inpic;
    //输出字符画数组
    array2d<char>* out_CharMatrix;
public:
    int DecNum;
    //FrameNum相关变量
    int PicNum;
    int FrameNum;
    int LongTermIdx;
    int LongTermPicNum;
    int FrameNumWrap;
    //POC相关变量
    int TopFieldOrderCnt;
    int BottomFieldOrderCnt;
    int PicOrderCntMsb;
    //内存控制标志
    uint8_t memory_management_control_operation;
    //这个值用来指明当前pic的预测状态， 用十进制位来表示
    //-1表示不用于预测，-10表示不存在，1 表示短期预测 10表示用于长期预测，
    int state_Ref;
    int LongTermFrameIdx;
    int get_POC(){return Min(TopFieldOrderCnt, BottomFieldOrderCnt);}
    bool is_IDR(){return type == 1 ? true : false;};
    bool is_UsedForShort(){return state_Ref % 10 == 1? true : false;};
    bool is_UsedForLong (){return state_Ref / 10 == 1 ? true : false;};
    bool is_UsedForRef  (){return state_Ref > 0 ? true : false;};
    //add a macroblock into the pic
    bool add_MB(int x, int y, macroblock* mb_ToAdd);
    
    //mode取值0 1 2，分别表示Y U V 的样点
    //注意这里的x和y，分别是
    //+--------- x
    //|
    //|
    //|y
    uint8_t get_SampleXY(int x, int y, int mode);
    //get the neighbour macroblock with the mode such as 'A' or 'B' etc
    macroblock* get_MBneighbour(macroblock* mb_curren, char direction);
    //return the macroblock* of the target, and the 4'th parmeter is the result index
    //mode : 
    //coeffType~:~0x100~~DC~~~|~0x200~AC~~|~0x000~default
    //colorType~:~0x010~~luma~|~0x020~Cb~~|~0x030~Cr
    //blockType~:~0x001~~4x4~~|~0x002~8x8~|~0x003~macroblock(16x16)   
    //if return indexResult == -1, it stands not avaiable  
    macroblock* get_BLneighbour(macroblock* mb_curren, char direction,int index_current, int mode,  int& indexResult);
    //return the macroblock* of the target, and the 4'th parmeter is the result index
    //mode : 
    //系数尺寸coeffType~:~0x100~~DC~~~~|~0x200~AC~~|~0x000~default~|
    //全块尺寸colorType~:~0x010~~16x16~|~0x020~8x8~|
    //样点尺寸blockType~:~0x001~~4x4~~~|~0x002~8x8~|~0x003~16x16~~~|~~0xRC004~RxC~|
    //if return indexResult == -1, it stands not avaiable
    //if return r or c == -1 it stands not avaiable
    //得到的r和c是在目标宏块中的相对坐标
    macroblock* get_BLneighbour(macroblock* mb_curren, char direction,int index_current, int mode,  int& indexResult, int& r, int& c);
    //当前宏块指针，方向，模式，宏块分区索引，子宏块分区索引，当前子宏块类型， 
    //最后两个参数是计算的结果分别是计算的宏块分区索引和子宏块分区索引的结果
    //模式取值：0x0F0，需要指明的只有全块的尺寸 16x16为 1 8x8为2
    //全块尺寸 colorType~:~0x010~~16x16~|~0x020~8x8~|
    macroblock* get_PartNeighbour(macroblock* mb_curren, char direction, int mode, int mbPartIdx, int subPartIdx, int& mbPartIdx_result , int& subPartIdx_result);
    //得到这个块的相邻块的运动矢量(在最后一个参数：矩阵指针指向的值 上返回 预测值)
    //返回的值暂时没有任何作用
    int get_MvNeighbour(macroblock* mb_curren, uint8_t mbPartIdx, uint8_t subMbPartIdx, int refIdxLX, uint8_t listSuffixFlag, class matrix* mv_lX);
    //format print the picture in macroblock
    friend std::ostream& operator<<(std::ostream& out ,picture& pic);

    bool chs_MbToOutmatrix();
    picture(int widthInSamples, int heightInsamples, int type);
    ~picture();
};
#endif