#include "functions.h"
#include "enums.h"
#include "staticcharts.h"
#include "macroblock.h"
bool Get_PredFlag(macroblock* m, uint8_t mbPartIdx, uint8_t flag)
{
    if(!m) return false;

    auto Pred_LX = flag ? Pred_L1 : Pred_L0;
    uint8_t premode = Get_MbPartPredMode(m, m->type, mbPartIdx);
    if(m->num_MBpart == 4 && m->sub_mb_type) 
    {
        premode = Get_SubMbPartPredMode(m->sub_mb_type[mbPartIdx]);
        if(premode == Pred_LX || premode == BiPred) return 1;
        else return 0;
    }
    else 
    {
        if(premode == Pred_LX || premode == BiPred) return 1;
        else return 0;  
    }
}
MbPartPredMode Get_MbPartPredMode(macroblock* ma, MbTypeName name, uint8_t a)
{
    if((ma->num_MBpart >= 1 && ma->num_MBpart <= 2) || ma->type == B_Direct_16x16)
    {
        if(name == I_NxN) return ma->transform_size_8x8_flag ? Intra_8x8 : Intra_4x4;
        else if(name > I_NxN && name <= I_PCM) return (MbPartPredMode)macroBlockInfo_I_slice[name][1];
        else if(name == SI_M) return (MbPartPredMode)macroBlockInfo_SI_slice[name -30][2 + a];
        else if(name >= B_Direct_16x16 && name <= B_Skip) return (MbPartPredMode)macroBlockInfo_B_slice[name - 50][2 + a];
        else if(name >= P_L0_16x16 && name <= P_Skip) return (MbPartPredMode)macroBlockInfo_P_SP_slice[name - 40][2 + a];
        else return Pred_NU;
    }
    else if(ma->num_MBpart == 4)
    {
        if(!ma->sub_mb_type) return Pred_NU;

        if(ma->type == B_Skip || ma->type == B_Direct_16x16) return Direct;
        else return (MbPartPredMode)Get_SubMbPartPredMode(ma->sub_mb_type[a]);
    }
    else return Pred_NU;
}
uint8_t Get_SubMbPartPredMode(SubMbTypeName name)
{
    if(name >= P_L0_8x8 && name <= P_L0_4x4) return subMbInfo[(uint8_t)name][2];
    else return subMbInfo[(uint8_t)(name - 10 + 5)][2];
}

uint8_t Get_NumMbPart(MbTypeName name)
{
    if(name >= B_Direct_16x16 && name <= B_Skip) return macroBlockInfo_B_slice[name - 50][1];
    else if(name >= P_L0_16x16 && name <= P_Skip) return macroBlockInfo_P_SP_slice[name - 40][1];
    else return 1;
}
uint8_t Get_MbPartWidth(MbTypeName mb_type)
{
    if(mb_type >= B_Direct_16x16 && mb_type <= B_Skip) return macroBlockInfo_B_slice[mb_type - 50][4];
    else if(mb_type >= P_L0_16x16 && mb_type <= P_Skip) return macroBlockInfo_P_SP_slice[mb_type - 40][4];
    return (uint8_t)255;
}
uint8_t Get_MbPartHeight(MbTypeName mb_type)
{
    if(mb_type >= B_Direct_16x16 && mb_type <= B_Skip) return macroBlockInfo_B_slice[mb_type - 50][5];
    else if(mb_type >= P_L0_16x16 && mb_type <= P_Skip) return macroBlockInfo_P_SP_slice[mb_type - 40][5];
    return (uint8_t)255;
}


uint8_t Get_SubMbPartHeight(SubMbTypeName sub_type)
{
    if(sub_type >= P_L0_8x8 && sub_type <= P_L0_4x4) return subMbInfo[sub_type][4];
    else if(sub_type >= B_Direct_8x8 && sub_type <= B_Bi_4x4) return subMbInfo[sub_type - 10 + 5][4];
    else return 255;
}
uint8_t Get_SubMbPartWidth(SubMbTypeName sub_type)
{
    if(sub_type >= P_L0_8x8 && sub_type <= P_L0_4x4) return subMbInfo[sub_type][3];
    else if(sub_type >= B_Direct_8x8 && sub_type <= B_Bi_4x4) return subMbInfo[sub_type - 10 + 5][3];
    else return 255;
}