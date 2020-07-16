#include "cabac.h"

#include "enums.h"
#include "NAL.h"
#include "picture.h"
#include <cmath>
#include "macroblock.h"
#include "block.h"
#include "residual.h"
#include "staticcharts.h"


#include "array2d.h"
#include "staticcharts.h"
#include "macroblock.h"
#include "slice.h"
#include "Parser.h"
#include <iostream>

#include "Debug.h"

#include "functions.h"




int cabac::cread_ae(int syntax)
{
    //result意思是计算出来的值
    uint32_t result;
    //初始化
    if(state == 0)
    {
        init_variable();
        init_engine();
        state = 1;
        if(debug->cabac_state_running()) printf(">>cabac: INIT!!!!!!\n");
    }
    result = Decode(syntax);
    //后处理
    if(syntax == 22 && (MbTypeName)result == I_PCM) {init_engine();}

    int a = syntax >> 12 & 0xFF;
    if(debug->cabac_result_ae())
    {
        if((a >= 0x61 && a <= 0x64 ) || (a == 0x41 || a == 0x43))
        printf(">>cabac: result of |%8x| is : |%5d|\n", syntax, result);
        else
        printf(">>cabac: result of |%8d| is : |%5d|\n", syntax, result);
    }

    return result;
}
//syntaxelements:
//
uint32_t cabac::Decode(int syntaxelements)
{ 
    uint32_t result = 0;

    if(syntaxelements == 0)  return 0;
    else if(syntaxelements == 11) result = read_mb_skip_flag();
    else if(syntaxelements == 21) result = read_mb_type();
    else if(syntaxelements == 22) result = read_transform_8x8_size_flag();
    else if(syntaxelements == 23) result = read_coded_block_pattern();
    else if(syntaxelements == 25) result = read_mb_qp_delta();
    else if(syntaxelements == 31) result = read_prev_intra4x4_pred_mode_flag();
    else if(syntaxelements == 32) result = read_rem_intra4x4_pred_mode();
    else if(syntaxelements == 35) result = read_intra_chroma_pred_mod();
    else if(syntaxelements == 51) result = read_sub_mb_type();
    else if(((syntaxelements >> 12) & 0xFF) == 0x41) result = read_ref_idx(syntaxelements);
    else if(((syntaxelements >> 12) & 0xFF) == 0x43) result = read_mvd_lx(syntaxelements);
    else if(((syntaxelements >> 12) & 0xFF) == 0x61) result = read_coded_block_flag(syntaxelements);
    else if(((syntaxelements >> 12) & 0xFF) == 0x62) result = read_significant_coeff_flag(syntaxelements);
    else if(((syntaxelements >> 12) & 0xFF) == 0x63) result = read_last_significant_coeff_flag(syntaxelements);
    else if(((syntaxelements >> 12) & 0xFF) == 0x64) result = read_coeff_abs_level_minus1(syntaxelements);
    else if(syntaxelements == 65) result = DecodeValueUsingCtxIdx(0, 1);
    else if(syntaxelements == 276) result = DecodeValueUsingCtxIdx(276, 0);

    return result;
}
bool cabac::DecodeValueUsingCtxIdx(uint16_t ctxIdx_value, uint8_t bypassFlag_value)
{
    pStateIdx = (*ctxIdxOfInitVariables)[ctxIdx_value][0];
    valMPS    = (*ctxIdxOfInitVariables)[ctxIdx_value][1];
    bool binVal;
    if(bypassFlag_value)//decode bypass
    {
        codIOffset = codIOffset << 1;
        codIOffset = codIOffset | p->read_bi();
        if(codIOffset >= codIRange) {binVal = 1; codIOffset = codIOffset - codIRange;}
        else binVal = 0;
        if(debug->cabac_state_running())     
        {
            printf(">>cabac: bypass decode\n");
            printf("         codIOffset: %5d\t", codIOffset);
            printf("          -------------------result : %d\n", binVal);
        }
    }
    else 
    {
        if(ctxIdx_value == 276)//decode terminate
        {
            codIRange = codIRange - 2;
            if(codIOffset >= codIRange) binVal = 1;
            else {binVal = 0;RenormD();}
            // if(debug->cabac_state_running())     
            // {
            //     printf(">>cabac: terminate decode\n");
            //     printf("         codIOffset: %5d\t", codIOffset);
            //     printf("          -------------------result : %d\n", binVal);
            // }
        }
        else//decode decision
        {
            uint16_t qCodIRangeIdx = (codIRange >> 6) & 3;
            uint16_t codIRangeLPS = rangeTabLPS[pStateIdx][qCodIRangeIdx];
            if(debug->cabac_state_running())     
            {
                printf("--cabac: Running State: ");
                printf("ctxIdx: %4d  ", ctxIdx_value);
                printf("codIOffset: %5d\t", codIOffset);
                printf("state: %5d\t", pStateIdx);
                printf("codIRang: %5d\t", codIRange);
                printf("codIRangeLPS: %5d\n", rangeTabLPS[pStateIdx][qCodIRangeIdx]);
                printf("         state(%3d)----->", pStateIdx);
                if(codIOffset >= codIRange)
                printf("MPS(%3d)\n", transIdxMPS[pStateIdx]);
                else
                printf("LPS(%3d)\n", transIdxLPS[pStateIdx]);

            }
            codIRange = codIRange - codIRangeLPS;
            if(codIOffset >= codIRange)
            {
                binVal = !(bool)valMPS;
                codIOffset -= codIRange;
                codIRange = codIRangeLPS;
                if(pStateIdx == 0) 
                    {(*ctxIdxOfInitVariables)[ctxIdx_value][1] = 1 - valMPS;};
                (*ctxIdxOfInitVariables)[ctxIdx_value][0] = transIdxLPS[pStateIdx];
            }
            else
            {
                binVal = (bool)valMPS;
                (*ctxIdxOfInitVariables)[ctxIdx_value][0] = transIdxMPS[pStateIdx];
            }
            RenormD();
        }
    }
    if(debug->cabac_result_bin()) printf(">>cabac: cur bin: %d\n", binVal);
    return binVal;
}
void cabac::RenormD()
{
    while(codIRange < 256)
    {
        codIRange  <<= 1;
        codIOffset <<= 1;
        codIOffset |= (uint8_t)(p->read_bi());
    }
}
//上下文变量的初始化
uint8_t cabac::init_variable()
{
    uint8_t ctxRangeCol = 0, mncol = 0;
    uint8_t preCtxState = 0;
    int m = 0, n = 0;
    ctxIdxOfInitVariables = new array2d<uint8_t>(1024, 2, 0);
    
    Slicetype lifeTimeSliceType = lifeTimeSlice->get_type();
    if(lifeTimeSliceType == I) {mncol = 0;}
    else {mncol = lifeTimeSlice->ps->cabac_init_idc + 1;}
 
    pStateIdx = 63; valMPS = 0;
    ctxIdxOfInitVariables->set_value(276, 0, pStateIdx);
    ctxIdxOfInitVariables->set_value(276, 1, valMPS);

    for (size_t i = 0; i < 43; i++)     //i is ctx  Idx range of current syntax element, init all 43 rows
    {

        if(lifeTimeSliceType == SI) {ctxRangeCol = 0;}
        else if(lifeTimeSliceType == I){ctxRangeCol = 1;}
        else if(lifeTimeSliceType == P || lifeTimeSliceType == SP) {ctxRangeCol = 2;}
        else {ctxRangeCol = 3;}
        
        for (size_t j = ctxIdxRangOfSyntaxElements[i][ctxRangeCol][0]; j <= ctxIdxRangOfSyntaxElements[i][ctxRangeCol][1]; j++)
        {   //j is ctxIdx
            m = mnValToCtxIdex[j][mncol][0];
            n = mnValToCtxIdex[j][mncol][1];
            if(!(ctxIdxRangOfSyntaxElements[i][ctxRangeCol][0] == 0 && ctxIdxRangOfSyntaxElements[i][ctxRangeCol][1] == 0))
            {
                preCtxState = (uint8_t)Clip3(1, 126,      ((m *  Clip3(0, 51, (int)lifeTimeSlice->ps->SliceQPY)) >> 4) + n         );
                if(preCtxState <= 63)
                { 
                    pStateIdx = 63 - preCtxState;
                    valMPS = 0;
                }
                else 
                {
                    pStateIdx = preCtxState - 64;
                    valMPS = 1;
                }
                ctxIdxOfInitVariables->set_value(j, 0, pStateIdx);
                ctxIdxOfInitVariables->set_value(j, 1, valMPS);
            }
        }
    }
    return 1;
}
uint8_t cabac::init_engine()
{
    codIRange = 510;
    codIOffset = p->read_un(9);
    return 1;
}

//返回的是值的索引，
int InBinarization(uint16_t result, int binIdx, const uint8_t binarization_chart[][2])
{
    int re = -1;
    uint8_t chart_length = 0;

    if(binarization_chart == binarization_mbtype_in_I) chart_length = 26;
    else if(binarization_chart == binarization_mbtype_in_PandSP) chart_length = 6;
    else /*mbtype B*/chart_length = 24;

    for(size_t i = 0; i < chart_length; i++)
        if(result == binarization_chart[i][0] && binarization_chart[i][1] == binIdx + 1){re = i; break;}
    return re;
}
int InBinarizationSub(uint16_t result, int binIdx, const uint8_t binarization_chart[][2])
{
    int re = -1;
    uint8_t chart_length = 0;

    if(binarization_chart == binarization_submbtype_in_PandSP) chart_length = 4;
    else /*if(binarization_chart == binarization_submbtype_in_B)*/ chart_length = 13;

    for(size_t i = 0; i < chart_length; i++)
        if(result == binarization_chart[i][0] && binarization_chart[i][1] == binIdx + 1){re = i; break;}
    return re;
}
bool cabac::Binarization_mbtype_submbtype(uint16_t &maxBinIdxCtx, int &ctxIdxOffset, uint8_t &bypassFlag)
{
    if(lifeTimeSlice->get_type() == I){maxBinIdxCtx = 6; ctxIdxOffset = 3; bypassFlag = 0;return true;}
    else if(lifeTimeSlice->get_type() == SI){return true;}
    else return false;
}
uint16_t cabac::read_mb_type()
{
    uint16_t result = 0;

    uint16_t ctxIdx_cur = 0;
    uint16_t result_cur = 0;
    uint16_t ctxIdxInc = 0;

    Slicetype slice_type = lifeTimeSlice->get_type();
    macroblock* cur = lifeTimeSlice->get_curMB();
    picture* p = pic;
    uint8_t ctxIdxOffset = 0;

    auto f = [p](macroblock* cur, char direction, uint16_t ctxIdxOffset)->uint8_t{
        macroblock* N = p->get_MBneighbour(cur, direction);
        uint8_t condTermFlagN = 0;
        if(!N || !cur->is_avaiable(N)) condTermFlagN = 0;
        else
        {
            if( (ctxIdxOffset == 0 && N->type == SI_M) ||\
                (ctxIdxOffset == 3 && N->type == I_NxN)||\
                (ctxIdxOffset == 27 && (N->type == B_Skip|| N->type == B_Direct_16x16))
            ) condTermFlagN  = 0;
            else condTermFlagN  = 1;
        }
        return condTermFlagN;
    };
    if(slice_type == I)
    {
        ctxIdxOffset = 3;
        int binIdx = -1;
        do
        {
            binIdx++;// sumi

            if(binIdx == 0) ctxIdxInc = f(cur, 'A', ctxIdxOffset) + f(cur, 'B', ctxIdxOffset);
            else if(binIdx == 1) {ctxIdx = 276;}
            else if(binIdx == 2 || binIdx == 3) ctxIdxInc = binIdx + 1;
            else if(binIdx == 4) ctxIdxInc = (b3 != 0) ? 5 : 6;
            else if(binIdx == 5) ctxIdxInc = (b3 != 0) ? 6 : 7;
            else ctxIdxInc = 7;
            if(binIdx != 1) ctxIdx = ctxIdxInc + ctxIdxOffset;
            result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
            if(binIdx == 3) b3 = result_cur;
            result_cur <<= binIdx;
            result += result_cur;
        } while((InBinarization(result, binIdx, binarization_mbtype_in_I)) == -1);
        result = InBinarization(result, binIdx, binarization_mbtype_in_I);

        return result;
    }
    else if(slice_type == P || slice_type == SP)
    {
        uint8_t prefix_ctxIdxOffset = 14;
        uint8_t suffix_ctxIdxOffset = 17;
        uint8_t prefix_result = 0;
        uint8_t suffix_result = 0;
        //求前缀
        int binIdx = -1;
        do
        {
            binIdx++;// sumi
            if(binIdx == 0 || binIdx == 1) ctxIdxInc = binIdx;
            else ctxIdxInc = (b1 != 1) ? 2 : 3;
            ctxIdx = ctxIdxInc + prefix_ctxIdxOffset;
            result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
            if(binIdx == 1) b1 = result_cur;
            result_cur <<= binIdx;
            prefix_result += result_cur;
        } while((InBinarization(prefix_result, binIdx, binarization_mbtype_in_PandSP)) == -1);
        prefix_result = InBinarization(prefix_result, binIdx, binarization_mbtype_in_PandSP);
        //如果前缀值不为5，那么说明是Pslcie中的P宏块
        if(prefix_result != 5){return prefix_result + 40;}
        //否则前缀值为5，说明是P片中的I宏块
        binIdx = -1;
        do
        {
            binIdx++;// sumi
             if(binIdx == 0) ctxIdxInc = 0;
            else if(binIdx == 1) {ctxIdx = 276;}
            else if(binIdx == 2 || binIdx == 3) ctxIdxInc = binIdx - 1;
            else if(binIdx == 4) ctxIdxInc = (b3 != 0) ? 2 : 3;
            else /*if(binIdx >= 5)*/ ctxIdxInc = 3;
            if(binIdx != 1) ctxIdx = ctxIdxInc + suffix_ctxIdxOffset;
            result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
            if(binIdx == 3) b3 = result_cur;
            result_cur <<= binIdx;
            suffix_result += result_cur;
        } while((InBinarization(suffix_result, binIdx, binarization_mbtype_in_I)) == -1);
        suffix_result = InBinarization(suffix_result, binIdx, binarization_mbtype_in_I);
        return suffix_result;
    }
    else if(slice_type == SI)
    {
        uint8_t prefix_ctxIdxOffset = 0;
        uint8_t suffix_ctxIdxOffset = 3;

    }
    else /*if(slice_type == B)*/
    {
        uint8_t prefix_ctxIdxOffset = 27;
        uint8_t suffix_ctxIdxOffset = 32;
        uint8_t prefix_result = 0;
        uint8_t suffix_result = 0;
        int binIdx = -1;
        do
        {
            binIdx++;// sumi
            if(binIdx == 0) ctxIdxInc = f(cur, 'A', prefix_ctxIdxOffset) + f(cur, 'B', prefix_ctxIdxOffset);
            else if(binIdx == 1) ctxIdxInc = 3;
            else if(binIdx == 2) ctxIdxInc = (b1 != 0) ? 4: 5;
            else ctxIdxInc = 5;
            ctxIdx = ctxIdxInc + prefix_ctxIdxOffset;
            result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
            if(binIdx == 1) b1 = result_cur;
            result_cur <<= binIdx;
            prefix_result += result_cur;
            if(binIdx > 7) exit(-1);
        } while((InBinarization(prefix_result, binIdx, binarization_mbtype_in_B)) == -1);
        prefix_result = InBinarization(prefix_result, binIdx, binarization_mbtype_in_B);
        //如果前缀值不为23，那么说明是Bslcie中的B宏块
        if(prefix_result != 23){return prefix_result + 50;}
        //否则前缀值为23，说明是B片中的I宏块
        binIdx = -1;
        do
        {
            binIdx++;// sumi
            if(binIdx == 0) ctxIdxInc = 0;
            else if(binIdx == 1) {ctxIdx = 276;}
            else if(binIdx == 2 || binIdx == 3) ctxIdxInc = binIdx - 1;
            else if(binIdx == 4) ctxIdxInc = (b3 != 0) ? 2 : 3;
            else /*if(binIdx >= 5)*/ ctxIdxInc = 3;
            if(binIdx != 1) ctxIdx = ctxIdxInc + suffix_ctxIdxOffset;
            result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
            if(binIdx == 3) b3 = result_cur;
            result_cur <<= binIdx;
            suffix_result += result_cur;
        } while((InBinarization(suffix_result, binIdx, binarization_mbtype_in_I)) == -1);
        suffix_result = InBinarization(suffix_result, binIdx, binarization_mbtype_in_I);
        return suffix_result;
    }
    return result;
}
uint8_t cabac::read_sub_mb_type()
{
    uint8_t result = 0;

    uint16_t ctxIdx_cur = 0;
    uint16_t result_cur = 0;
    uint16_t ctxIdxInc = 0;

    Slicetype slice_type = lifeTimeSlice->get_type();
    int ctxIdxOffset = 0;
    if(slice_type == P || slice_type == SP)
    {// offset = 21
        ctxIdxOffset = 21;
        int binIdx = -1;
        do
        {
            binIdx++;// sumi
            ctxIdxInc = binIdx;
            ctxIdx_cur = ctxIdxInc + ctxIdxOffset;
            result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx_cur, 0);
            result_cur <<= binIdx;
            result += result_cur;
        } while((InBinarizationSub(result, binIdx, binarization_submbtype_in_PandSP)) == -1);
        result = InBinarizationSub(result, binIdx, binarization_submbtype_in_PandSP);
    }
    else// if(slice_type == B)
    {// offset = 36
        ctxIdxOffset = 36;
        int binIdx = -1;
        do
        {
            binIdx++;// sumi
            if(binIdx == 0){ctxIdxInc = 0;}
            else if(binIdx == 1) {ctxIdxInc = 1;}
            else if(binIdx == 2) ctxIdxInc = (b1 != 0) ? 2 : 3;
            else ctxIdxInc = 3;

            
            ctxIdx_cur = ctxIdxInc + ctxIdxOffset;
            result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx_cur, 0);
            if(binIdx == 1) b1 = result_cur;
            result_cur <<= binIdx;
            result += result_cur; 
        } while((InBinarizationSub(result, binIdx, binarization_submbtype_in_B)) == -1);
        result = InBinarizationSub(result, binIdx, binarization_submbtype_in_B);
    }
    //I slice 不可能出现子宏块的情况

    return result;
}
uint8_t cabac::read_transform_8x8_size_flag()
{
    uint8_t result_cur = 0;
    uint16_t result = 0;
    
    macroblock* currentMB = lifeTimeSlice->get_curMB();
    macroblock* A = NULL, * B = NULL;
    A = pic->get_MBneighbour(currentMB, 'A');
    B = pic->get_MBneighbour(currentMB, 'B');
    uint8_t  condTermFlagA = 0,  condTermFlagB = 0;

    if(!A || A->transform_size_8x8_flag == 0) condTermFlagA = 0;
    else condTermFlagA = 1;
    if(!B || B->transform_size_8x8_flag == 0) condTermFlagB = 0;
    else condTermFlagB = 1;

    ctxIdxInc = condTermFlagA + condTermFlagB;
    ctxIdx = ctxIdxInc + 399;
    result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
    result = result_cur;

    return result;
}

uint16_t cabac::read_coded_block_pattern()
{
    //prefix  and suffix
    uint8_t prefix_ctxIdxOffset = 73, suffix_ctxIdxOffset = 77;
    //maxBin:                     3                          1
    //prefix: FL cMax = 15
    
    
    uint8_t result_cur = 0;
    uint16_t result = 0;
    macroblock* currentMB = lifeTimeSlice->get_curMB();
    macroblock* A = NULL, * B = NULL;
    //prefix
    uint16_t prefix_ctxIdxInc = 0;
    uint16_t prefix_result = 0;
    int binIdx = -1;
    int tmp;

    uint8_t  condTermFlagA = 0,  condTermFlagB = 0;
    tmp = 0;
    do
    {
        binIdx++;// sumi
        A = pic->get_BLneighbour(currentMB, 'A', binIdx, 0x012, tmp);
        if(!A || A->type == I_PCM ||\
          (A != currentMB && (A->type != P_Skip || A->type != B_Skip) && ((A->CodedBlockPatternLuma >> tmp) & 1) != 0)||\
          (A == currentMB && (((prefix_result >> tmp) & 0x1) != 0)))
            condTermFlagA = 0;
        else condTermFlagA = 1;
        B = pic->get_BLneighbour(currentMB, 'B', binIdx, 0x012, tmp);
        if(!B || B->type == I_PCM ||\
          (B != currentMB && (B->type != P_Skip || B->type != B_Skip) && ((B->CodedBlockPatternLuma >> tmp) & 1) != 0)||\
          (B == currentMB && (((prefix_result >> tmp) & 0x1) != 0)))
            condTermFlagB = 0;
        else condTermFlagB = 1;
        prefix_ctxIdxInc = condTermFlagA + 2 * condTermFlagB;
        ctxIdx = prefix_ctxIdxInc + prefix_ctxIdxOffset;
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur = result_cur << binIdx;
        prefix_result += result_cur;
    } while(binIdx < 3);

    uint16_t suffix_ctxIdxInc = 0;
    uint16_t suffix_result = 0;
    tmp = 0;
    binIdx = -1;
    do
    {
        binIdx++;// sumi
        A = pic->get_MBneighbour(currentMB, 'A');
        if(A && A->type == I_PCM) condTermFlagA = 1;
        else if((!A || A->type == P_Skip || A->type == B_Skip) ||\
                (binIdx == 0 && A->CodedBlockPatternChroma == 0) ||\
                (binIdx == 1 && A->CodedBlockPatternChroma != 2))
            condTermFlagA = 0;
        else condTermFlagA = 1;
        B = pic->get_MBneighbour(currentMB, 'B');
        if(B && B->type == I_PCM) condTermFlagB = 1;
        else if((!B || B->type == P_Skip || B->type == B_Skip) ||\
                (binIdx == 0 && B->CodedBlockPatternChroma == 0) ||\
                (binIdx == 1 && B->CodedBlockPatternChroma != 2))
            condTermFlagB = 0;
        else condTermFlagB = 1;
        suffix_ctxIdxInc = condTermFlagA + 2 * condTermFlagB + ((binIdx == 1) ? 4 : 0);
        ctxIdx = suffix_ctxIdxInc + suffix_ctxIdxOffset;
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur = result_cur << binIdx;
        suffix_result += result_cur;
    } while(IsIn_TU_binarization(suffix_result, 2, binIdx) == -1);
    suffix_result = IsIn_TU_binarization(suffix_result, 2, binIdx);
    suffix_result <<= 4;
    result = prefix_result + suffix_result;


    return result;

    //suffix: TU cMax = 2;
    
}
int8_t cabac::read_mb_qp_delta()
{
    int syntaxRequest = 0;
    uint16_t  ctxIdx_cur = 0;
    uint64_t result_cur = 0;
    uint64_t result = 0;
    int isResult = 0;
    int binIdx = -1;
    do
    {
        binIdx++;// sumi
        ctxIdx_cur = DecodeCtxIdxUsingBinIdx(binIdx, 2, 60, 0);
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx_cur, 0);
        // printf(">>cabac: mb_qp_delta_cur :%d\n", result_cur);
        result_cur <<= binIdx;
        result += result_cur;
    } while((isResult = IsIn_U_binarization(result, binIdx)) == -1);
    result = isResult;
    result = pow((-1), (result + 1)) * (ceil((double)result / 2));
    
    return result;
    
}
uint8_t cabac::read_significant_coeff_flag(int syntaxelement)
{
    //get ctxBlockCat and maxNumCoeff
    uint8_t ctxBlockCat = (syntaxelement >> 8) & 0xF;
    uint8_t maxNumCoeff = coded_blokc_flag_maxNumCoeff[ctxBlockCat];
    uint8_t levelListIdx = syntaxelement >> 20;

    //get ctxIdxOffset
    uint16_t ctxIdxOffset = 0;
    if(ctxBlockCat < 5) ctxIdxOffset = 105;
    else if(ctxBlockCat == 5) ctxIdxOffset = 402;
    else if(ctxBlockCat > 5 && ctxBlockCat < 9) ctxIdxOffset = 484;
    else if(ctxBlockCat == 9) ctxIdxOffset = 660;
    else if(ctxBlockCat > 9 && ctxBlockCat < 13) ctxIdxOffset = 528;
    else if(ctxBlockCat == 13) ctxIdxOffset = 718;
    else if(ctxBlockCat == 5 || ctxBlockCat == 9 || ctxBlockCat == 13) ctxIdxOffset = 1012;

    if(ctxBlockCat != 3 && ctxBlockCat != 5 && ctxBlockCat != 9 && ctxBlockCat != 13) ctxIdxInc = levelListIdx;
    else if(ctxBlockCat == 3) ctxIdxInc = Min(levelListIdx / 1 , 2);
    else ctxIdxInc = ctxIncForCtxBlockCat[levelListIdx][0];

    ctxIdx = (int)ctxIdxInc + (int)ctxIdxBlockCatOffsetOfctxBlockCat[1][ctxBlockCat] + (int)ctxIdxOffset;
    uint8_t result = DecodeValueUsingCtxIdx(ctxIdx, 0);
    return result;

}
uint8_t cabac::read_last_significant_coeff_flag(int syntaxelement)
{
 

    //get ctxBlockCat and maxNumCoeff
    uint8_t ctxBlockCat = (syntaxelement >> 8) & 0xF;
    uint8_t maxNumCoeff = coded_blokc_flag_maxNumCoeff[ctxBlockCat];
    uint8_t levelListIdx = syntaxelement >> 20;

    //get ctxIdxOffset
    uint16_t ctxIdxOffset = 0;
    if(ctxBlockCat < 5) ctxIdxOffset = 166;
    else if(ctxBlockCat == 5) ctxIdxOffset = 417;
    else if(ctxBlockCat > 5 && ctxBlockCat < 9) ctxIdxOffset = 572;
    else if(ctxBlockCat == 9) ctxIdxOffset = 690;
    else if(ctxBlockCat > 9 && ctxBlockCat < 13) ctxIdxOffset = 616;
    else if(ctxBlockCat == 13) ctxIdxOffset = 748;
    else if(ctxBlockCat == 5 || ctxBlockCat == 9 || ctxBlockCat == 13) ctxIdxOffset = 1012;

    if(ctxBlockCat != 3 && ctxBlockCat != 5 && ctxBlockCat != 9 && ctxBlockCat != 13) ctxIdxInc = levelListIdx;
    else if(ctxBlockCat == 3) ctxIdxInc = Min(levelListIdx / 1 , 2);
    else ctxIdxInc = ctxIncForCtxBlockCat[levelListIdx][2];
    
    ctxIdx = (int)ctxIdxInc + (int)ctxIdxBlockCatOffsetOfctxBlockCat[2][ctxBlockCat] + (int)ctxIdxOffset;

    uint8_t result = DecodeValueUsingCtxIdx(ctxIdx, 0);

    return result;
}
uint8_t cabac::read_coded_block_flag(int syntaxelement)
{
    //get ctxBlockCat and maxNumCoeff

    uint8_t ctxBlockCat = (syntaxelement >> 8) & 0xF;
    uint8_t maxNumCoeff = coded_blokc_flag_maxNumCoeff[ctxBlockCat];
    uint8_t iCbCr = (syntaxelement >> 4) & 0xF;
    uint8_t index = (syntaxelement) & 0xF;

    uint8_t colorType = 0;
    if(iCbCr == 3) colorType = 1;
    else colorType = iCbCr + 2;
    colorType <<= 4;

    uint8_t blockType = 0;
    switch (ctxBlockCat)
    {
    case 0:blockType = 0x003;break;
    case 1:blockType = 0x001;break;
    case 2:blockType = 0x001;break;
    case 3:blockType = 0x001;break;
    case 4:blockType = 0x001;break;
    case 5:blockType = 0x002;break;
    default:break;
    }
    
    //get ctxIdxOffset
    uint16_t ctxIdxOffset = 0;
    if(ctxBlockCat < 5) ctxIdxOffset = 85;
    else if(ctxBlockCat > 5 && ctxBlockCat < 9) ctxIdxOffset = 460;
    else if(ctxBlockCat > 9 && ctxBlockCat < 13) ctxIdxOffset = 472;
    else if(ctxBlockCat == 5 || ctxBlockCat == 9 || ctxBlockCat == 13) ctxIdxOffset = 1012;
    
    //get tranblock
    macroblock* currentMB = lifeTimeSlice->get_curMB();
    int index_A = 0;
    macroblock* mbAddrA = pic->get_BLneighbour(currentMB, 'A', index, colorType + blockType, index_A);
    block*  transBlockA;    
    int index_B = 0;
    macroblock* mbAddrB = pic->get_BLneighbour(currentMB, 'B', index, colorType + blockType, index_B);
    block*  transBlockB;

    
    if(ctxBlockCat == 0 || ctxBlockCat == 6 || ctxBlockCat == 10)
    {  
        auto f = [iCbCr, ctxBlockCat](macroblock* cur, macroblock* N , int index_N)->block*{
            block* transBlockN = NULL;
            if((N && cur->is_avaiable(N)) && N->premode == Intra_16x16)
                {
                    if(ctxBlockCat == 0) transBlockN = N->re->luma[1];
                    else if(ctxBlockCat == 3) transBlockN = N->re->chroma[0]->get_childBlock(iCbCr);
                    else if(ctxBlockCat == 4) transBlockN = N->re->chroma[1]->get_childBlock(iCbCr)->get_childBlock(index_N);
                }
            else transBlockN = NULL;
            return transBlockN;
        };
        transBlockA = f(currentMB, mbAddrA, index_A);
        transBlockB = f(currentMB, mbAddrB, index_B);
    }
    else if(ctxBlockCat == 1 || ctxBlockCat == 2)
    {
        auto f = [iCbCr](macroblock* cur, macroblock* N, int index_N)->block*{
            block* transBlockN = NULL;
            if(cur->is_avaiable(N) && (N->type != P_Skip && N->type != B_Skip && N->type != I_PCM) && ((N->CodedBlockPatternLuma >>(index_N >> 2)) & 1) != 0 && N->transform_size_8x8_flag == 0)
            transBlockN = N->re->luma[0]->get_childBlock(index_N/4)->get_childBlock(index_N%4);
            else if(cur->is_avaiable(N) && (N->type != P_Skip && N->type != B_Skip && N->type != I_PCM) && ((N->CodedBlockPatternLuma >>(index_N >> 2)) & 1) != 0 && N->transform_size_8x8_flag == 1)
                transBlockN = NULL;//这里没有认真写，因为fox.264不会出现8x8变换解码
            else transBlockN = NULL;
            return transBlockN;
        };
        transBlockA = f(currentMB, mbAddrA, index_A);
        transBlockB = f(currentMB, mbAddrB, index_B);      
    }
    else if(ctxBlockCat == 3)
    {
        mbAddrA = pic->get_MBneighbour(currentMB, 'A');
        mbAddrB = pic->get_MBneighbour(currentMB, 'B');
        auto f = [iCbCr](macroblock* cur, macroblock* mbAddrN)->block*{
            block* transBlockN = NULL;
            if(mbAddrN && (mbAddrN->type != P_Skip && mbAddrN->type != B_Skip && mbAddrN->type != I_PCM) && mbAddrN->CodedBlockPatternChroma != 0)
                transBlockN = mbAddrN->re->chroma[0]->get_childBlock(iCbCr);
            else transBlockN = NULL;
            return transBlockN;
        };
        transBlockA = f(currentMB, mbAddrA);
        transBlockB = f(currentMB, mbAddrB);
    }
    else if(ctxBlockCat == 4)
    {
        if(mbAddrA && (mbAddrA->type != P_Skip && mbAddrA->type != B_Skip && mbAddrA->type != I_PCM) && mbAddrA->CodedBlockPatternChroma == 2)
            transBlockA = mbAddrA->re->chroma[1]->get_childBlock(iCbCr)->get_childBlock(index_A);
        else transBlockA = NULL;

        if(mbAddrB && (mbAddrB->type != P_Skip && mbAddrB->type != B_Skip && mbAddrB->type != I_PCM) && mbAddrB->CodedBlockPatternChroma == 2)
            transBlockB = mbAddrB->re->chroma[1]->get_childBlock(iCbCr)->get_childBlock(index_B);
        else transBlockB = NULL;
    }
    //后面的ctxBlockCat都没有写，因为从5开始都是用于444的

    //get condTermFlagA and condTermFlagB
    uint8_t condTermFlagA, condTermFlagB;
    Slice* sl = lifeTimeSlice;
    auto f_condTermFlagN = [sl](macroblock* cur, macroblock* N, block* transBlockN)->uint8_t{
        uint8_t condTermFlagN = 0;

        if((!cur->is_avaiable(N) && cur->is_interpred())||\
        (cur->is_avaiable(N) && transBlockN == NULL && N && N->type != I_PCM)||\
        ((cur->is_intrapred() && N && N->is_interpred()) && (sl->uppernal->type >= 2 && sl->uppernal->type <= 4))
        ) 
        condTermFlagN = 0;
        else if((!cur->is_avaiable(N) && cur->is_intrapred())||\
                (N && N->type == I_PCM)) 
                condTermFlagN = 1;
        else condTermFlagN = transBlockN->coded_block_flag;

        return condTermFlagN;
    };
    //calc ctxIdxInc
    uint8_t ctxIdxInc;
    ctxIdxInc = f_condTermFlagN(currentMB, mbAddrA, transBlockA) + 2 * f_condTermFlagN(currentMB, mbAddrB, transBlockB);

    //get ctxIdxOffsetCat
    uint8_t ctxIdxOffsetCat = ctxIdxBlockCatOffsetOfctxBlockCat[0][ctxBlockCat];
    //calc ctxIdx
    ctxIdx = ctxIdxInc + ctxIdxOffsetCat + ctxIdxOffset;

    //calc bin;
    uint8_t result =  DecodeValueUsingCtxIdx(ctxIdx, 0);

    return result;
}
uint8_t cabac::read_coeff_sign_flag(int syntaxelement)
{
    uint8_t result = DecodeValueUsingCtxIdx(0, 1);
    return result;
}
uint32_t cabac::read_coeff_abs_level_minus1(int syntaxelement)
{
    uint8_t ctxBlockCat = (syntaxelement >> 8) & 0xF;

    int binIdx = -1;
    //当前的binIdx解出来的ctxIdx
    uint16_t result = 0;
    //当前位上的数据
    uint32_t result_cur = 0;
    //当前解出来的二进制串的数据
    uint32_t result_str = 0;
    //当前的结果
    int64_t prefix_result;
    int64_t suffix_result;
    //总的offset
    uint8_t ctxIdxOffset = 0;

    //求前缀

    //求总offset的前缀   最大只有两位
    uint16_t prefix_ctxIdxOffset = 0;
    if(ctxBlockCat < 5) prefix_ctxIdxOffset = 227;
    else if(ctxBlockCat == 5) prefix_ctxIdxOffset = 417;
    else if(ctxBlockCat > 5 && ctxBlockCat < 9) prefix_ctxIdxOffset = 864;
    else if(ctxBlockCat == 9) prefix_ctxIdxOffset = 690;
    else if(ctxBlockCat > 9 && ctxBlockCat < 13) prefix_ctxIdxOffset = 908;
    else if(ctxBlockCat == 13) prefix_ctxIdxOffset = 748;


    int numDecodAbsLevelEq1 = (syntaxelement >> 24) & 0xF;
    int numDecodAbsLevelGt1 = (syntaxelement >> 20) & 0xF;
    //求前缀二值串
    binIdx = -1;
    do
    {
        binIdx++;// sumi
        //求cat索引的ctxIdxInc
        uint8_t ctxIdxInc = 0;
        if(binIdx == 0) ctxIdxInc = ((numDecodAbsLevelGt1 != 0) ? 0: Min(4, (1 + numDecodAbsLevelEq1)));
        else ctxIdxInc = 5 + Min(4 - ((ctxBlockCat == 3)?1 : 0), numDecodAbsLevelGt1);

        ctxIdx = ctxIdxInc + prefix_ctxIdxOffset + ctxIdxBlockCatOffsetOfctxBlockCat[3][ctxBlockCat];
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur = result_cur << binIdx;
        result_str += result_cur;
    } while( ((prefix_result = IsIn_TU_binarization(result_str, 14, binIdx)) == -1) && (binIdx < 13) );
    
    if(debug->cabac_result_bin()) cout << ">>cabac: break line -------------------------------for prefix & suffix" << endl;
    //求后缀二值串，旁路解码
    uint8_t binIdx2 = -1;
    result_cur = 0;
    result_str = 0;
    //如果前缀不足14位或者是一个14位的截断数，那么没有后缀
    if(binIdx < 13 || (binIdx == 13 && prefix_result == 13)){suffix_result = 0;}
    else if(prefix_result == 14)
    {
        do
        {
            binIdx2++;// sumi
            result_cur = (uint8_t)DecodeValueUsingCtxIdx(0, 1);
            result_str <<= 1;
            result_str += result_cur;
        } while((suffix_result = IsIn_UEGk_binarization(result_str, binIdx2, 0, 0, 0)) == -1);
    }
    result = suffix_result + prefix_result;

    return result;
}

uint16_t cabac::read_intra_chroma_pred_mod()
{
    int syntaxRequest;
    uint16_t ctxIdxInc;
    uint16_t result_cur = 0;
    uint16_t result_all = 0;
    uint16_t result = 0;

    uint8_t condTermFlagA = 0;
    uint8_t condTermFlagB = 0;
    macroblock* cur   = lifeTimeSlice->get_curMB();
    macroblock* A     = pic->get_MBneighbour(cur, 'A');
    macroblock* B     = pic->get_MBneighbour(cur, 'B');
    auto f = [](macroblock* cur, macroblock* N)->uint8_t{
        uint8_t condTermFlagN;
        if(!cur->is_avaiable(N) ||\
        N->is_interpred() || \
        N->type == I_PCM || \
        N->intra_chroma_pred_mode == 0
        )  condTermFlagN = 0;
        else condTermFlagN = 1;
        return condTermFlagN;
    };
    int binIdx = -1;
    do
    {
        binIdx++;// sumi
        if(binIdx == 0)
        {
            
            condTermFlagA = f(cur, A);
            condTermFlagB = f(cur, B);
            ctxIdxInc = condTermFlagA + condTermFlagB;   
        }
        else ctxIdxInc = 3;
        ctxIdx =  ctxIdxInc + 64;
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur = result_cur << binIdx;
        result_all += result_cur;
    } while(IsIn_TU_binarization(result_all, 3, binIdx) == -1);
    result = IsIn_TU_binarization(result_all, 3, binIdx);
    return result;
}

uint8_t cabac::read_prev_intra4x4_pred_mode_flag()
{
    //FL cMax = 1   offset = 68
    uint16_t ctxIdx_cur = 0 + 68;
    uint8_t result = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx_cur, 0);
    return result;
}
uint8_t cabac::read_rem_intra4x4_pred_mode()
{
    //FL cMax = 7   offset = 69
    uint16_t ctxIdx = 0 + 69;
    uint8_t result = 0;
    uint8_t result_cur = 0;
    int binIdx = -1;
    do
    {
        binIdx++;
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur <<= binIdx;
        result += result_cur;
    } while (binIdx < 2);
    return result;
}

uint8_t cabac::read_mb_skip_flag()
{
    uint8_t result = 0;
    
    ctxIdxOffset = 0;

    Slicetype slice_type = lifeTimeSlice->get_type();
    if(slice_type == B) ctxIdxOffset = 24;
    else /*if(slice_type == P || slice_type == SP)*/ ctxIdxOffset = 11;
    macroblock* cur = lifeTimeSlice->get_curMB();
    macroblock* A = pic->get_MBneighbour(cur, 'A');
    macroblock* B = pic->get_MBneighbour(cur, 'B');
    auto f = [](macroblock* cur,  macroblock* N)->uint8_t{
        uint8_t condTermFlagN = 0; 
        if(!cur->is_avaiable(N) || N->mb_skip_flag == 1) 
            condTermFlagN = 0;
        else condTermFlagN = 1;
        return condTermFlagN;
    };
    ctxIdx = f(cur, A) + f(cur, B) + ctxIdxOffset;
    result = DecodeValueUsingCtxIdx(ctxIdx, 0);

    return result;
}



//句法元素结构：    0xAVVI
//最低位是预测的方向(最低位的1上是时间，2是空间)，上面的两位是句法元素标志值，再往上一位是宏块分区的索引，再往上一位是句法元素MbaffFrameFlag的值
uint8_t cabac::read_ref_idx(int syntaxelements)
{
    //U offset 54    
    uint16_t result_cur = 0;
    uint16_t result_all = 0;
    uint16_t result = 0;

    uint8_t temporalDirection = (syntaxelements) & 0x1;
    uint8_t spatialDirection  = (syntaxelements >> 1) & 0x1;
    uint8_t mbPartIdx         = (syntaxelements >> 8) & 0xF;
    //subIdx 左移4位去0xF的并
    uint8_t MbaffFrameFlag    = (syntaxelements >> 20) & 0xF;
    
    uint16_t ctxIdxInc = 0;
    uint16_t ctxIdxOffset = 54;
    picture* p = this->pic;

    macroblock* cur   = lifeTimeSlice->get_curMB();
    MbPartPredMode Pred_L = temporalDirection ? Pred_L1 : Pred_L0;
    if(cur->id_slice == 4 && cur->position_x == 6 && cur->position_y == 44)
        int a = 0;
    auto f = [p,mbPartIdx,Pred_L,spatialDirection, temporalDirection,MbaffFrameFlag](macroblock* cur, char direction)->uint8_t{
        int tmp = 0;
        int mbPartIdxN = 0;
        uint8_t fieldflag = 0;//if(MbaffFrameFlag && cur是帧宏块 && N 是场宏块)
        macroblock* N = p->get_PartNeighbour(cur, direction, 0x010, mbPartIdx, 0, mbPartIdxN, tmp);
        uint8_t predModeEqualFlagN = 0;
        uint8_t refIdxZeroFlagN = 0;
        if(N)
        {
            if(N->is_interpred())
            {
                if(temporalDirection)refIdxZeroFlagN = (N->ref_idx_l1[mbPartIdxN] > fieldflag)?0:1;
                else refIdxZeroFlagN = (N->ref_idx_l0[mbPartIdxN] > fieldflag)?0:1;
            }
            
            if(N->type == B_Direct_16x16 || N->type == B_Skip) predModeEqualFlagN = 0;
            else if(N->type == P_8x8 || N->type == B_8x8)
            {
                MbPartPredMode partpremode = (MbPartPredMode)Get_SubMbPartPredMode(N->sub_mb_type[mbPartIdxN]);
                if(partpremode != Pred_L && partpremode != BiPred) predModeEqualFlagN = 0;
                else predModeEqualFlagN = 1;
            }
            else
            {
                MbPartPredMode partpremode = Get_MbPartPredMode(N, N->type, mbPartIdxN);
                if(partpremode != Pred_L && partpremode != BiPred) predModeEqualFlagN = 0;
                else predModeEqualFlagN = 1;
            }
        }
        uint8_t condTermFlagN = 0;
        if(!cur->is_avaiable(N) || \
           (N->type == P_Skip && N->type == B_Skip) || \
           N->is_intrapred() || \
           predModeEqualFlagN == 0 || \
           refIdxZeroFlagN == 1
        ) condTermFlagN = 0;
        else condTermFlagN = 1;
        return condTermFlagN;
    };
    int binIdx = -1;
    do
    {
        binIdx++;// sumi

        if(binIdx == 0)
        {
            uint8_t condTermFlagA = 0, condTermFlagB = 0;
            condTermFlagA = f(cur, 'A');
            condTermFlagB = f(cur, 'B');
            ctxIdxInc = condTermFlagA + 2 * condTermFlagB;   
        }
        else if(binIdx == 1) ctxIdxInc = 4;
        else ctxIdxInc = 5;

        ctxIdx =  ctxIdxInc + ctxIdxOffset;
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur = result_cur << binIdx;
        result_all += result_cur;
    } while(IsIn_U_binarization(result_all, binIdx) == -1);
    result = IsIn_U_binarization(result_all, binIdx);
    return result;
}
//子块索引1 + 子宏块子块索引1  + 句法2 + 方向1
int cabac::read_mvd_lx(int syntaxelements)
{
    //编码模式UEG3
    //signedValFlag=1, uCoff=9  pre offset 40
    uint8_t signedValFlag = 1;
    uint16_t result_cur = 0;
    uint16_t result_all = 0;
    int result = 0;
    
    uint8_t temporalDirection = (syntaxelements) & 0x1;
    uint8_t spatialDirection  = (syntaxelements >> 1) & 0x1;
    uint8_t subMbPartIdx      = (syntaxelements >> 4) & 0xF;
    uint8_t mbPartIdx         = (syntaxelements >> 8) & 0xF;
    uint8_t MbaffFrameFlag    = (syntaxelements >> 20) & 0xF;
    
    macroblock* cur   = lifeTimeSlice->get_curMB();
    MbPartPredMode Pred_L = temporalDirection ? Pred_L1 : Pred_L0;
    picture* p = this->pic;

    //求前缀
    uint8_t prefix_ctxIdxOffset = 0;

    int64_t prefix_result = 0;
    int64_t suffix_result = 0;
    //如果是水平预测，偏移是40，否则47
    if(spatialDirection == 0) prefix_ctxIdxOffset = 40;
    else prefix_ctxIdxOffset = 47;


    auto f = [p,mbPartIdx,subMbPartIdx,Pred_L, temporalDirection, spatialDirection, MbaffFrameFlag](macroblock* cur,int mbPartIdxN, char direction)->uint16_t{
        int subMbPartIdxN = 0;
        uint8_t fieldflag = 0;//if(MbaffFrameFlag && cur是帧宏块 && N 是场宏块)
        macroblock* N = p->get_PartNeighbour(cur, direction, 0x010, mbPartIdx, subMbPartIdx, mbPartIdxN, subMbPartIdxN);
        matrix* mvd_lX = NULL;
        if(N) mvd_lX = temporalDirection ? N->mvd_l1 : N->mvd_l0; 

        uint8_t predModeEqualFlagN = 0;
        if(N)
        {
            if(N->type == B_Direct_16x16 || N->type == B_Skip) predModeEqualFlagN = 0;
            else if(N->type == P_8x8 || N->type == B_8x8)
            {
                if((Get_SubMbPartPredMode(N->sub_mb_type[mbPartIdxN])) != Pred_L && Get_SubMbPartPredMode(N->sub_mb_type[mbPartIdxN]) != BiPred) predModeEqualFlagN = 0;
                else predModeEqualFlagN = 1;
            }
            else
            {
                MbPartPredMode partpremode = Get_MbPartPredMode(N, N->type, mbPartIdxN);
                if(partpremode != Pred_L && partpremode != BiPred) predModeEqualFlagN = 0;
                else predModeEqualFlagN = 1;
            }
        }
        uint16_t absMvdCompN = 0;
        if( !cur->is_avaiable(N) || \
            (N->type == P_Skip || N->type == B_Skip) || \
            N->is_intrapred() || \
            predModeEqualFlagN == 0  )
        {absMvdCompN = 0;}
        else
        {
            //这俩还有两个帧场的条件
            absMvdCompN = Abs(mvd_lX[mbPartIdxN][subMbPartIdxN][spatialDirection]);
        }
        return absMvdCompN;
    };
    uint8_t ctxIdxInc = 0;
    int binIdx = -1;
    do
    {
        binIdx++;// sumi
        if(binIdx == 0)
        {
            uint16_t absMvdCompA = f(cur,mbPartIdx, 'A');
            uint16_t absMvdCompB = f(cur,mbPartIdx, 'B');
            uint16_t absMvdComp = absMvdCompA + absMvdCompB;   
            if(absMvdComp < 3) ctxIdxInc = 0;
            else if(absMvdComp > 32) ctxIdxInc = 2;
            else ctxIdxInc = 1;
        }
        else if(binIdx >= 1 && binIdx <= 3) ctxIdxInc = binIdx + 2;
        else ctxIdxInc = 6;
        // if(cur->id_slice == 3 && cur->position_x == 10 && cur->position_y == 28 && mbPartIdx == 1)
        //     ctxIdxInc = 2;
        ctxIdx =  ctxIdxInc + prefix_ctxIdxOffset;
        result_cur = (uint8_t)DecodeValueUsingCtxIdx(ctxIdx, 0);
        result_cur = result_cur << binIdx;
        prefix_result += result_cur;
    } while(IsIn_TU_binarization(prefix_result, 9 , binIdx) == -1);
    prefix_result = IsIn_TU_binarization(prefix_result, 9 , binIdx);
    //求后缀，旁路解码 
    uint8_t binIdx2 = -1;
    result_cur = 0;
    uint32_t result_str = 0;
    //如果前缀不足9位或者是一94位的截断数，那么没有后缀
    if((binIdx < 8 || (binIdx == 8 && prefix_result == 8))){suffix_result = 0;}
    else if(prefix_result == 9 )
    {
        do
        {
            binIdx2++;// sumi
            result_cur = (uint8_t)DecodeValueUsingCtxIdx(0, 1);
            result_str <<= 1;
            result_str += result_cur;
        } while((suffix_result = IsIn_UEGk_binarization(result_str, binIdx2, 0, 0, 3)) == -1);
    }
    int sig = 1;
    if(prefix_result != 0 && signedValFlag)
        sig = (DecodeValueUsingCtxIdx(0,1) == 1) ? -1 : 1;
    result = sig * (suffix_result + prefix_result);
    return result;
}


int cabac::IsIn_U_binarization(uint64_t value, uint8_t binIdx)
{
    uint32_t a = 0;
    while((value & 0x1) == 1)
    {
        value >>= 1;
        a++;
    }
    if(binIdx == a) return binIdx; 
    else return -1;
}
int cabac::IsIn_TU_binarization(uint32_t value, uint8_t cMax, uint8_t binIdx)
{
    int a = 0;
    if(binIdx + 1 == cMax) 
    {
        while ((value & 0x1) == 1)
        {
            value >>= 1;
            a++;
        }
        if(a == cMax || (a + 1 == cMax)) return a;
        else return -1;
    }
    else return IsIn_U_binarization(value, binIdx);
}
int cabac::IsIn_UEGk_binarization(uint32_t value, uint8_t binIdx, uint8_t  signedValFlag, uint8_t uCoeff, uint8_t k)
{
    uint8_t length  = binIdx + 1;
    //求前缀的长度：
    uint8_t prefix_length = uCoeff;
    uint8_t prefix = 0;
    //求前缀
    if(prefix_length != 0)
    {
        for(uint8_t i = 0; i < prefix_length; i++)
        {(prefix <<= 1) |= (value & (0x1 << i));}
        if(IsIn_TU_binarization(prefix, uCoeff, prefix_length) != -1)
        {
            prefix = IsIn_TU_binarization(prefix, uCoeff, prefix_length);
            if(prefix < uCoeff) return prefix;
        }
        else return -1;
    }

    //calc suffix of UEGK
    uint32_t suffix = value >> prefix_length;
    //calc suffix length
    uint8_t suffix_length = length - prefix_length;
    
    //data structure:  v1 + 0 + v2
    uint8_t start_1_count = 0;
    uint32_t count_1_pointer = 1 << (suffix_length - 1);
    uint8_t sigLength = signedValFlag;
    uint32_t v1 = 0;
    uint32_t v2 = 0;
    for (uint8_t i = 0; i < suffix_length; i++)
    {
        if(suffix & count_1_pointer)
        {
            count_1_pointer >>= 1;
            start_1_count++;
        }
        else break;
    }
    if(start_1_count + 1 + (start_1_count + k) + sigLength != suffix_length) return -1;
    else
    {
        v1 = (1 << (start_1_count)) - 1;
        v2 = suffix - (v1 << (1 + start_1_count + k));
        v1 <<= k;
        suffix = v1 + v2;
    }
    if(signedValFlag) suffix *= -1;
    else suffix *= 1;
    return prefix + suffix;
}


//binIdx maxBinIdxCtx ctxIdxOffset syntaxRequest
uint16_t cabac::DecodeCtxIdxUsingBinIdx(uint16_t binIdx, uint16_t maxBinIdxCtx, int ctxIdxOffset, int syntaxRequest)
{
    uint16_t ctxIdx_result, ctxIdxInc;
    uint8_t condTermFlagA = 0;
    uint8_t condTermFlagB = 0;
    macroblock* mb_cur = lifeTimeSlice->get_curMB();
    macroblock* mb_neiA = pic->get_MBneighbour(mb_cur, 'A');
    macroblock* mb_neiB = pic->get_MBneighbour(mb_cur, 'B');


    if(ctxIdxOffset == 70)
    { //upper
        if(!mb_neiB || ((mb_neiB && mb_neiB->is_avaiable(mb_cur)) || mb_neiB->mb_skip_flag == 0 )) condTermFlagB = 0;
        else
        {
            if(mb_neiB->mb_field_decoding_flag == 0) condTermFlagB = 0;
            else condTermFlagB  = 1;
        }
        //left
        if(!mb_neiA || ((mb_neiA && mb_neiA->is_avaiable(mb_cur)) || mb_neiA->mb_skip_flag == 0 )) condTermFlagA = 0;
        else
        {
            if(mb_neiA->mb_field_decoding_flag == 0) condTermFlagA = 0;
            else condTermFlagA  = 1;
        }
        ctxIdxInc = condTermFlagA + condTermFlagB;
    }
    //mb_type for i slice only
    else if(ctxIdxOffset == 3)
    {
        if(binIdx == 0)
        {
            if(!mb_neiB || ( mb_neiB && mb_neiB->is_avaiable(mb_cur) ) ) condTermFlagB = 0;
            else
            {
                if( (ctxIdxOffset == 0 && mb_neiB->type == SI_M) ||\
                    (ctxIdxOffset == 3 && mb_neiB->type == I_NxN)||\
                    (ctxIdxOffset == 27 && (mb_neiB->type == B_Skip|| mb_neiB->type == B_Direct_16x16))
                ) condTermFlagB  = 0;
                else condTermFlagB  = 1;
            }
            //left
            if(!mb_neiA || ( mb_neiA && mb_neiA->is_avaiable(mb_cur) ) ) condTermFlagA = 0;
            else
            {
                if( (ctxIdxOffset == 0 && mb_neiA->type == SI_M) ||\
                    (ctxIdxOffset == 3 && mb_neiA->type == I_NxN)||\
                    (ctxIdxOffset == 17 && (mb_neiA->type == B_Skip|| mb_neiA->type == B_Direct_16x16))
                ) condTermFlagA  = 0;
                else condTermFlagA  = 1;
            }
            ctxIdxInc = condTermFlagA + condTermFlagB;
        }
        else if(binIdx == 1) {return ctxIdx_result = 276;}
        else if(binIdx == 2 || binIdx == 3) ctxIdxInc = binIdx + 1;
        else if(binIdx == 4) ctxIdxInc = (b3 != 0) ? 5 : 6;
        else if(binIdx == 5) ctxIdxInc = (b3 != 0) ? 6 : 7;
        else ctxIdxInc = 7;
    }
    //coded_block_pattern------prefix(luma)
    else if(binIdx >= 0 && binIdx <= 3 && ctxIdxOffset == 73)
    {
        ctxIdxInc = condTermFlagA + 2 * condTermFlagB + ((binIdx == 1) ? 4 : 0);
    }
    //coded_block_pattern------suffix(Chroma)

    //mb_qp_delta
    else if(ctxIdxOffset == 60)
    {
        if(binIdx == 0)
        {
            macroblock* currentMB = lifeTimeSlice->get_curMB();
            macroblock*  prevMB = lifeTimeSlice->get_mbUsingIdInSlice(currentMB->idx_inslice - 1);
            if((prevMB == NULL || (prevMB->type == P_Skip || prevMB->type == B_Skip)) ||\
                prevMB->type == I_PCM||\
            (prevMB->premode != Intra_16x16 && (prevMB->CodedBlockPatternChroma == 0 && prevMB->CodedBlockPatternLuma == 0)) ||\
            prevMB->mb_qp_delta == 0
            )  ctxIdxInc = 0;
            else ctxIdxInc = 1;
        }
        else if(binIdx == 1) {ctxIdxInc = 2;}
        else {ctxIdxInc = 3;}
    }
    //ref idx l0 and l1
    else if(binIdx == 0 && ctxIdxOffset == 54)
    {

    }
    //intra_chroma_pred_mode 
    else if(ctxIdxOffset == 64)
    {

    }
    //coded_block_flag 
    //else if()
    // transform_size_8x8_flag 
    else if (ctxIdxOffset == 399)
    {
        if((mb_neiB == NULL || mb_neiB->is_avaiable(mb_cur))||\
            mb_neiB->transform_size_8x8_flag == 0\
          )condTermFlagB = 0;
        else condTermFlagB = 1;
        if((mb_neiA == NULL || mb_neiA->is_avaiable(mb_cur))||\
           mb_neiA->transform_size_8x8_flag == 0\
          )condTermFlagA = 0;
        else condTermFlagA = 1;
        ctxIdxInc = condTermFlagA + condTermFlagB;

    }

    ctxIdx_result = ctxIdxInc + ctxIdxOffset;
    return ctxIdx_result;
}








bool cabac::slice_end()
{
    Sdelete_s(ctxIdxOfInitVariables);
    if(state == 1) state = 0;

    return true;
}
bool cabac::set_pic(picture* pic1){this->pic = pic1; return true;}
bool cabac::set_slice(Slice* sl){this->lifeTimeSlice = sl; return true;}

cabac::cabac(Parser* parser)
{
    b3 = 0;
    b1 = 0;
    state = 0;
    this->p = parser;
    this->debug = parser->debug;
}
cabac::~cabac()
{
    Sdelete_s(ctxIdxOfInitVariables);
}