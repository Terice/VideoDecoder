#include "picture.h"

#include "macroblock.h"
#include "array2d.h"
#include "matrix.h"
#include <stdio.h>
#include <iostream>
#include "staticcharts.h"

#include "Charsample.h"

#include "functions.h"

void picture::print_complete()
{
    //宏块是否齐全的输出
    printf(">>pic  :\n");
    for (size_t i = 0; i < this->height_mb; i++)
    {
        if(i%4 == 0) printf("%-2lu\n", i/4 * 4);
        for (size_t j = 0; j < this->width_mb; j++)
        {
            if(j%4 == 0) printf(" | ");
            if(this->mb_inpic->get_value_xy(i, j) != NULL) printf("O");
            else printf("~");
        }
        printf(" |\n");
    }
    printf("=>pic  :");
}
std::ostream& operator<<(std::ostream& out ,picture& pic)
{
    //字符画的输出
    printf("%s", pic.out_CharMatrix->data);
    return out;
}
macroblock* picture::get_MBneighbour(macroblock* mb_curren, char direction)
{
    int x = 0, y = 0;
    if(direction == 'B'){x = mb_curren->position_x - 1;y = mb_curren->position_y;}
    else if(direction == 'A'){x = mb_curren->position_x;y = mb_curren->position_y - 1;}
    else if(direction == 'D'){x = mb_curren->position_x - 1; y = mb_curren->position_y - 1;}
    else {x = mb_curren->position_x - 1; y = mb_curren->position_y + 1;}// 'C'

    if(this->mb_inpic->is_avaiable(x, y)){return (*mb_inpic)[x][y];}
    else return NULL;
}
bool picture::add_MB(int x, int y, macroblock* mb_ToAdd)
{
    if(mb_inpic->is_avaiable(x, y)) {mb_inpic->set_value(x, y, mb_ToAdd); return true;}
    else return false;
}
bool picture::chs_MbToOutmatrix()
{
    //可以指定的字符画宽高比例
    int hei_scal = 4;
    int wid_scal = 2;
    //
    out_CharMatrix = new array2d<char>(height_mb * 16 / hei_scal + 1, width_mb * 16 / wid_scal + 1, 0);
    char tmp = 0;
    for (size_t y = 0; y < height_mb * 16; y += hei_scal)
    {
        for (size_t x = 0; x < width_mb * 16; x+= wid_scal)
        {
            //除以10拿到字符画单元的范围，25- 表示把亮度的高低值颠倒
            tmp = out_char[25 - (get_SampleXY(x, y, 0) / 10)];
            (*out_CharMatrix)[y / hei_scal][x / wid_scal] = tmp;
        } 
        //每一行的左后一个像素后面加上换行符
        (*out_CharMatrix)[y / hei_scal][width_mb * 16 / wid_scal] = '\n';
    }
    //每一帧的结尾最后一个字符指定为结束符
    (*out_CharMatrix)[height_mb * 16 / hei_scal][width_mb * 16 / wid_scal] = '\0';

    return true;
}
void position_FromCurUpperLeft(int& x, int& y, uint8_t cur_width, uint8_t cur_height, uint8_t all_width, uint8_t all_height)
{

}
void position_Transform(int& x, int& y, int x_in, int y_in, char direction)
{

}

inline void position_ToN(int xN, int yN, int& xW, int& yW, int maxW, int maxH)
{
    xW = (xN + maxW) % maxW;
    yW = (yN + maxH) % maxH;
}
//return the macroblock* of the target, and the 4'th parmeter is the result index
//mode : 
//系数尺寸coeffType~:~0x100~~DC~~~~|~0x200~AC~~|~0x000~default~|
//全块尺寸colorType~:~0x010~~16x16~|~0x020~8x8~|
//样点尺寸blockType~:~0x001~~4x4~~~|~0x002~8x8~|~0x003~16x16~~~|~~0xWH004~WxH~|
//if return indexResult == -1, it stands not avaiable
macroblock* picture::get_BLneighbour(macroblock* mb_current, char direction, int index_current, int mode,  int& indexResult)
{
    int r = 0, c = 0;
    return  get_BLneighbour(mb_current, direction, index_current, mode, indexResult, r, c);
}
macroblock* picture::get_BLneighbour(macroblock* mb_current, char direction,int index_current, int mode,  int& indexResult, int& r, int& c)
{
    /*宏块的坐标轴
    0__> x
    |
    V y
    */
    int8_t xD = 0, yD = 0;//相邻块坐标差
    int8_t xN = 0, yN = 0;//计算的坐标相对于目标宏块的差
    int xW = 0, yW = 0;//计算的坐标在宏块中的绝对位置
    int xP = 0, yP = 0;//计算的坐标在宏块中的绝对位置
    //得到基本的配置信息
    int blockType = mode & 0xF;//宏块分类的依据，块分为多少个，
    int colorType = (mode >> 4) & 0xF;//宏块分类的依据，宏块的全高度、全宽度是多少
    int coeffType = (mode >> 8) & 0xF;

    uint8_t indexBlockSideWidth = 0, indexBlockSideHeight = 0;//索引块高和块宽
    uint8_t sampleBlockSideWidth = 0, sampleBlockSideHeight = 0;;//样点块高和宽
    macroblock* R = NULL;

    int maxW = 16, maxH = 16;//全块的宽度和高度，以样点为单位
    if(colorType == 1) {maxW = 16, maxH = 16;}//如果是Y块，全块的样点长度的最大长度为16, 
    else {maxW = 8, maxH = 8;} //Cb Cr块,全高全宽为8 (4:2:0)

    //确定样点块的宽度和高度
    if(blockType == 1) sampleBlockSideHeight = sampleBlockSideWidth = 4;
    else if(blockType == 2) sampleBlockSideHeight = sampleBlockSideWidth = 8;
    else if(blockType == 3) sampleBlockSideHeight = sampleBlockSideWidth = 16;
    else /*blockType == 4自定义的样点块的宽和高*/ {sampleBlockSideWidth = ((blockType >> 16) & 0xF); sampleBlockSideHeight = ((blockType >> 12) & 0xF);}
    //确定索引块的宽，高
    indexBlockSideWidth = maxW / sampleBlockSideWidth;
    indexBlockSideHeight = maxH / sampleBlockSideHeight;

    //确定在所取方向上的坐标偏移
    switch (direction)
    {
    case 'A':xD = -1;   yD =  0; break;
    case 'B':xD =  0;   yD = -1; break;
    case 'C':xD = sampleBlockSideWidth; yD = -1; break;
    case 'D':xD = -1;   yD = -1; break;
    default:break;
    }

    //如果是4x4块在16x16中的索引，那么需要逆扫描
    if(blockType == 1 && colorType == 1) index_current = block4x4Index[index_current];
    //索引换成样点块内样点的坐标并且加上偏移 得到 相对坐标
    xN = (index_current % indexBlockSideWidth) * sampleBlockSideWidth + xD;
    yN = (index_current / indexBlockSideHeight) * sampleBlockSideHeight + yD;
    
    //目标宏块为自己的情况只有三种，一种是求左边的A，x >=0 ,另一种是求上边B y >=0， 还有是 x >= 0 , y >= 0;
    //确定宏块是哪一个以及是不是当前宏块
    if     (xN >= 0 && yN >= 0 && xN < maxW)    R = mb_current;
    else if(xN <  0 && yN <  0)                 R = get_MBneighbour(mb_current, 'D');
    else if(xN <  0 && yN >= 0)                 R = get_MBneighbour(mb_current, 'A');
    else if(xN >= 0 && yN <  0 && xN < maxW)    R = get_MBneighbour(mb_current, 'B');
    else{
        if (xN >= maxW && yN <  0)              R = get_MBneighbour(mb_current, 'C');
        else                                    R = NULL;
    }
    //相对坐标换成目标宏块内坐标
    position_ToN(xN, yN, xW, yW, maxW, maxH);
    //目标宏块内坐标换成索引并输出
    if(!mb_current->is_avaiable(R))
    {
        indexResult = -1;
        r = -1;
        c = -1;
        return R = NULL;
    }
    else
    {
        xP = xW;
        yP = yW;
        r = yP;
        c = xP;
        indexResult = indexBlockSideHeight * (yP / sampleBlockSideHeight) + (xP / sampleBlockSideWidth);
        //如果是luma4x4块，那么换到4x4索引
        //其他的情况都是遵循扫描顺序的，所以可以直接返回数组的索引
        if(blockType == 1 && colorType == 1) indexResult = block4x4Index[indexResult];
        return R;
    }
}

void Index_ToN(int xP, int yP, MbTypeName type, SubMbTypeName* sub_type, int& mbPartIdx, int& subMbPartIdx)
{
    if(type >= I_NxN && type <= I_PCM) mbPartIdx = 1;
    else mbPartIdx = (16 / Get_MbPartWidth(type)) * (yP / Get_MbPartHeight(type)) + (xP / Get_MbPartWidth(type));

    if(type != P_8x8 && type != P_8x8ref0 && type != B_8x8 && type != B_Skip && type != B_Direct_16x16)
        subMbPartIdx = 0;
    else if(type == B_Skip || type == B_Direct_16x16)
        subMbPartIdx = 2 * ((yP % 8) / 4) + ((xP % 8) / 4);
    else 
        subMbPartIdx = ( 8 / Get_SubMbPartWidth(sub_type[mbPartIdx])) * ((yP % 8) / Get_SubMbPartHeight(sub_type[mbPartIdx])) + ((xP % 8) / Get_SubMbPartWidth(sub_type[mbPartIdx]));

}

macroblock* picture::get_PartNeighbour(macroblock* mb_current, char direction, int mode, int mbPartIdx , int subPartIdx, int& mbPartIdx_result , int& subPartIdx_result)
{
    /*宏块的坐标轴
    0----> x
    |
    V y
    */
    int x = 0, y = 0;   //初始坐标
    int8_t xD = 0, yD = 0;//相邻块坐标差
    int8_t xN = 0, yN = 0;//计算的坐标相对于目标宏块的差
    int xW = 0, yW = 0;//计算的坐标在宏块中的绝对位置
    int xS = 0, yS = 0;//

    SubMbTypeName* current_sub = mb_current->sub_mb_type;
    int colorType = (mode >> 4) & 0xF;//宏块分类的依据，宏块的全高度、全宽度是多少
    int maxW = 16, maxH = 16;//全块的宽度和高度，以样点为单位
    if(colorType == 1) {maxW = 16, maxH = 16;}//如果是Y块，全块的样点长度的最大长度为16, 
    else {maxW = 8, maxH = 8;} //Cb Cr块,全高全宽为8 (4:2:0)
    uint8_t indexBlockSideWidth = maxW / Get_MbPartWidth(mb_current->type);
    uint8_t indexBlockSideHeight = maxH / Get_MbPartHeight(mb_current->type);
    //逆扫描得到子块起始坐标
    x = (mbPartIdx % indexBlockSideWidth) * Get_MbPartWidth(mb_current->type);
    y = (mbPartIdx / indexBlockSideWidth) * Get_MbPartHeight(mb_current->type);

    //计算得到子子块的起始坐标
    if(mb_current->type == P_8x8 || mb_current->type == P_8x8ref0 || mb_current->type == B_8x8) {xS = (subPartIdx % 8) * 8; yS = (subPartIdx / 8) * 8;}
    else {xS = 0; yS = 0;}
    
    //求解预测块的宽度
    uint8_t predPartWidth = 0;
    if(mb_current->type == P_Skip || mb_current->type == B_Skip || mb_current->type == B_Direct_16x16) predPartWidth = 16;
    else if(mb_current->type == B_8x8)
    {
        if(current_sub[mbPartIdx] == B_Direct_8x8) predPartWidth = 16;
        else predPartWidth = Get_SubMbPartWidth(current_sub[mbPartIdx]);
    }
    else if(mb_current->type == P_8x8 || mb_current->type == P_8x8ref0) predPartWidth = Get_SubMbPartWidth(current_sub[mbPartIdx]);
    else predPartWidth = Get_MbPartWidth(mb_current->type);

    switch (direction)
    {
    case 'A':xD = -1;   yD =  0; break;
    case 'B':xD =  0;   yD = -1; break;
    case 'C':xD = predPartWidth; yD = -1; break;
    case 'D':xD = -1;   yD = -1; break;
    default:break;
    }

    //子块起始坐标+子宏块子块起始坐标 = 宏块内坐标
    //加上偏移得到目标宏块内的坐标
    xN = x + xS + xD;
    yN = y + yS + yD;
    //确定宏块是哪一个以及是不是当前宏块
    macroblock* R;
    if     (xN >= 0 && yN >= 0 && xN < maxW)    R = mb_current;
    else if(xN <  0 && yN <  0)                 R = get_MBneighbour(mb_current, 'D');
    else if(xN <  0 && yN >= 0)                 R = get_MBneighbour(mb_current, 'A');
    else if(xN >= 0 && yN <  0 && xN < maxW)    R = get_MBneighbour(mb_current, 'B');
    else{
        if (xN >= maxW && yN <  0)              R = get_MBneighbour(mb_current, 'C');
        else                                    R = NULL;
    }
    
    //得到目标宏块内的坐标
    position_ToN(xN, yN, xW, yW, maxW, maxH);

    //目标宏块内坐标换成索引并输出
    if(!mb_current->is_avaiable(R))
    {
        mbPartIdx = -1;
        subPartIdx = -1;
    }
    else
    {
        MbTypeName type_N = R->type;
        SubMbTypeName* type_sub_N = R->sub_mb_type;
        Index_ToN(xW, yW, type_N, type_sub_N, mbPartIdx_result, subPartIdx_result);
    }
    return R;
}

int picture::get_MvNeighbour(macroblock* mb_curren, uint8_t mbPartIdx, uint8_t subMbPartIdx, uint8_t listSuffixFlag, matrix* mv_lX)
{
    macroblock* A = NULL;int mbPartIdx_A = -1;int subMbPartIdx_A = -1;int refIdxLX_A = -1;int mv_lx_A[2];
    macroblock* B = NULL;int mbPartIdx_B = -1;int subMbPartIdx_B = -1;int refIdxLX_B = -1;int mv_lx_B[2];
    macroblock* C = NULL;int mbPartIdx_C = -1;int subMbPartIdx_C = -1;int refIdxLX_C = -1;int mv_lx_C[2];
    macroblock* D = NULL;int mbPartIdx_D = -1;int subMbPartIdx_D = -1;int refIdxLX_D = -1;int mv_lx_D[2];
    
    MbPartPredMode Pred_LX = listSuffixFlag ? Pred_L1 : Pred_L0;
    int* ref_idx_lx = listSuffixFlag ? mb_curren->ref_idx_l1 : mb_curren->ref_idx_l0;
    int refIdxLX = ref_idx_lx[mbPartIdx];
    auto f_1 = [&mb_curren, listSuffixFlag, mbPartIdx, subMbPartIdx,this, Pred_LX](char direction, int& mbPartIdx_N, int& subMbPartIdx_N, int& refIdxLX_N, int mv_lx_N[2])->macroblock*{
        MbPartPredMode premode_cur = Get_MbPartPredMode(mb_curren, mb_curren->type, mbPartIdx);
        MbPartPredMode premode_cur_sub = mb_curren->sub_mb_type ? (MbPartPredMode)Get_SubMbPartPredMode(mb_curren->sub_mb_type[mbPartIdx]) : Pred_NU;
        
        macroblock* N =  get_PartNeighbour(mb_curren, direction, 0x010, mbPartIdx, subMbPartIdx, mbPartIdx_N, subMbPartIdx_N);
        
        uint8_t predFlagLX = Get_PredFlag(N, mbPartIdx_N, listSuffixFlag);
        int refIdx = -1;
        matrix* mv_lx = NULL;
        int* ref_idx_lx = NULL;
        if(N)
        {
            ref_idx_lx = listSuffixFlag ? N->ref_idx_l1 : N->ref_idx_l0;
            mv_lx = listSuffixFlag ? N->mv_l1 : N->mv_l0;
        }
        //如果 N 不可用 || 是帧内 || 不预测
        if(!mb_curren->is_avaiable(N) || N->is_intrapred() || !predFlagLX)
        {
            refIdx = -1;
            mv_lx_N[0] = 0;
            mv_lx_N[1] = 0;
        }
        else
        {
            refIdx = ref_idx_lx[mbPartIdx_N];
            mv_lx_N[0] = mv_lx[mbPartIdx_N][subMbPartIdx_N][0];
            mv_lx_N[1] = mv_lx[mbPartIdx_N][subMbPartIdx_N][1];
        }
        refIdxLX_N = refIdx;
        return N;
    };
    A = f_1('A', mbPartIdx_A, subMbPartIdx_A, refIdxLX_A, mv_lx_A);
    B = f_1('B', mbPartIdx_B, subMbPartIdx_B, refIdxLX_B, mv_lx_B);
    C = f_1('C', mbPartIdx_C, subMbPartIdx_C, refIdxLX_C, mv_lx_C);
    D = f_1('D', mbPartIdx_D, subMbPartIdx_D, refIdxLX_D, mv_lx_D);
    //初始化完成
    if(mb_curren->type == P_Skip && \
        (!mb_curren->is_avaiable(A) || (A->is_interpred() && (A->ref_idx_l0[0] && A->mv_l0[0][0][0] == 0 && A->mv_l0[0][0][1] == 0)) ||\
           !mb_curren->is_avaiable(B) || (B->is_interpred() && (B->ref_idx_l0[0] && B->mv_l0[0][0][0] == 0 && B->mv_l0[0][0][1] == 0)) \
        )
    )
    {//Skip宏块直接置零返回
        mv_lX[0][0][0] = 0;
        mv_lX[0][0][1] = 0;
        return 1;
    }
    uint8_t partWidth = Get_MbPartWidth(mb_curren->type);
    uint8_t partHeight= Get_MbPartHeight(mb_curren->type);
    if(!mb_curren->is_avaiable(C) || mbPartIdx_C == -1 || subMbPartIdx_C == -1)
    {
        C = D;
        mbPartIdx_C = mbPartIdx_D;
        subMbPartIdx_C = subMbPartIdx_D;
        mv_lx_C[0] = mv_lx_D[0];
        mv_lx_C[1] = mv_lx_D[1];
    }
    //这里的mv_lx在修改的时候，只加上了预测的值（之后加上mvd得到实际的运动矢量），因为这个预测值即没有用于读取也没有用于预测，所以就不多开空间了，
    if(partWidth == 16 && partHeight == 8 && mbPartIdx == 0 && refIdxLX_B == refIdxLX) 
    {
        mv_lX[mbPartIdx][subMbPartIdx][0] += mv_lx_B[0];
        mv_lX[mbPartIdx][subMbPartIdx][1] += mv_lx_B[1];
    }
    else if((partWidth == 16 && partHeight == 8 && mbPartIdx == 1 && refIdxLX_A == refIdxLX) || \
            (partWidth == 8 && partHeight == 16 && mbPartIdx == 0 && refIdxLX_A == refIdxLX)
            )
    {
        mv_lX[mbPartIdx][subMbPartIdx][0] += mv_lx_A[0];
        mv_lX[mbPartIdx][subMbPartIdx][1] += mv_lx_A[1];
    }
    else if(partWidth == 8 && partHeight == 16 && mbPartIdx == 1 && refIdxLX_C == refIdxLX)
    {
        mv_lX[mbPartIdx][subMbPartIdx][0] += mv_lx_C[0];
        mv_lX[mbPartIdx][subMbPartIdx][1] += mv_lx_C[1];
    }
    else //中值处理
    {
        if(!mb_curren->is_avaiable(B) && !mb_curren->is_avaiable(C))
        {
            mv_lx_B[0] = mv_lx_C[0] = mv_lx_A[0];
            mv_lx_B[1] = mv_lx_C[1] = mv_lx_A[1];
            refIdxLX_B = refIdxLX_C = refIdxLX_A;
        }
        if     (refIdxLX_A == refIdxLX && refIdxLX_B != refIdxLX && refIdxLX_C != refIdxLX)
        {
            mv_lX[mbPartIdx][subMbPartIdx][0] += mv_lx_A[0];
            mv_lX[mbPartIdx][subMbPartIdx][1] += mv_lx_A[1];
        }
        else if(refIdxLX_A != refIdxLX && refIdxLX_B == refIdxLX && refIdxLX_C != refIdxLX)
        {
            mv_lX[mbPartIdx][subMbPartIdx][0] += mv_lx_B[0];
            mv_lX[mbPartIdx][subMbPartIdx][1] += mv_lx_B[1];
        }
        else if(refIdxLX_A != refIdxLX && refIdxLX_B != refIdxLX && refIdxLX_C == refIdxLX)
        {
            mv_lX[mbPartIdx][subMbPartIdx][0] += mv_lx_C[0];
            mv_lX[mbPartIdx][subMbPartIdx][1] += mv_lx_C[1];
        }
        else 
        {
            mv_lX[mbPartIdx][subMbPartIdx][0] += Median(mv_lx_A[0], mv_lx_B[0], mv_lx_C[0]);
            mv_lX[mbPartIdx][subMbPartIdx][1] += Median(mv_lx_A[1], mv_lx_B[1], mv_lx_C[1]);
        }
    }
    return 1;
}

uint8_t picture::get_SampleXY(int x, int y, int mode)
{
    int r_mb = y / 16;   //|--------- x
    int c_mb = x / 16;   //|
    int r_sa = y % 16;   //|
    int c_sa = x % 16;   //|y
    
    if(mode == 0)return      (*(((*mb_inpic)[r_mb][c_mb])\
                                                            ->sample_Y))[r_sa][c_sa];
    else if(mode == 1)return (*(((*mb_inpic)[r_mb][c_mb])\
                                                            ->sample_U))[r_sa][c_sa];
    else return              (*(((*mb_inpic)[r_mb][c_mb])\
                                                            ->sample_V))[r_sa][c_sa];
}
picture::picture(int widthInMB, int heightInMB, int type)
{
    static int count = -1;
    count++;

    DecNum = count;
    this->type = type;
    out_CharMatrix = NULL;

    mb_inpic = new array2d<macroblock*>(heightInMB,widthInMB, NULL);
    
    height_mb = heightInMB;
    width_mb = widthInMB;


    state_Ref = Nun_ref;
    POC = 0;
    PicNum = 0;
    LongTermPicNum = -1;

    FrameNum = 0;
    LongTermIdx = -1;
    FrameNumWrap = 0;
    TopFieldOrderCnt = 0;
    BottomFieldOrderCnt = 0;
    memory_management_control_operation = 0;
    LongTermFrameIdx = -1;

    state_Out = false;
}
picture::~picture()
{
    if(mb_inpic)
    {
        for (uint i = 0; i < height_mb * width_mb; i++)
        {
            Sdelete_s(mb_inpic->get_value_i(i));
        }
        Sdelete_s(mb_inpic);
    }
    
    Sdelete_s(out_CharMatrix);
}