#include "Debug.h"
#include <stdio.h>

#include "macroblock.h"
#include "slice.h"
#include "Parser.h"
#include "staticcharts.h"
#include "residual.h"
#include "block.h"
#include "picture.h"
#include "matrix.h"
#include "NAL.h"
#include "enums.h"
#include "functions.h"

#include "Decoder.h"
#include <cmath>
void macroblock::Calc(int mode)
{
    //如果是帧内预测
    if(is_intrapred())
    {
        if(premode == Intra_16x16)
        {
            this->Decode(mode);
            this->ConstructPicture();
        }
        //4x4需要循环16次解码和重建的过程
        else if(premode == Intra_4x4)
        for (uint8_t i = 0; i < 16; i++)
        {
            this->Decode(block4x4Index[i]);
            this->ConstructPicture();
        }
    }
    else //if(is_interpred())
    {
        //帧间也是4x4解码残差 
        this->Decode(0);
        this->ConstructPicture();
    }


}

void macroblock::Info()
{
    //解码完毕打印信息并且退出宏块
    if(parser_g->debug->macroblock_all())
    {
        printf(">>MB  :  type: %2d\n", mb_type);
        printf(">>MB  :  QPY: %2d QPC: %2d, %2d\n", QPY_, QPC_, QPC_);
        printf("      :  position: X:%2d Y:%2d\n", this->position_x, this->position_y);
        printf("      :  SliceID: %2d\n", this->id_slice);
    }
    if(parser_g->debug->prediction_result_Y())
    cout << "--predicte :Y" << endl << (*pred_Y) << endl;

    if(parser_g->debug->residual_result_Y())
    cout << "--residual :Y" << endl << *(re->residual_Y) << endl;
    if(parser_g->debug->residual_result_Cb())
    cout << "--residual :U" << endl << *(re->residual_U) << endl;
    if(parser_g->debug->residual_result_Cr())
    cout << "--residual :V" << endl << *(re->residual_V) << endl;

    if(parser_g->debug->conspic_result_Y())
    cout << "--sample: Construct Picture:Y : " << endl << *sample_Y << endl;
    if(parser_g->debug->conspic_result_Cb())
    cout << "--sample: Construct Picture:U: " << endl << *sample_U << endl;
    if(parser_g->debug->conspic_result_Cr())
    cout << "--sample: Construct Picture:V: " << endl << *sample_V << endl;

    if(parser_g->debug->macroblock_all())
    {
        cout << "=>MB |<<\n" << endl;
    }
}
void macroblock::Init0(int mode)
{
    if(up_slice->get_type() == P || up_slice->get_type() == SP) type = P_Skip;
    else type = B_Skip;
    num_MBpart = Get_NumMbPart(type);

    if(type == P_Skip)
    {
        mvd_l0       = new matrix[num_MBpart];
        for (uint8_t i = 0; i < num_MBpart; i++){matrix init(1,2,0); mvd_l0[i] << init;}
        mvd_l1       = new matrix[num_MBpart];
        for (uint8_t i = 0; i < num_MBpart; i++){matrix init(1,2,0); mvd_l1[i] << init;}
        mv_l0       = new matrix[num_MBpart];
        for (uint8_t i = 0; i < num_MBpart; i++){matrix init(1,2,0); mv_l0[i] << init;}
        mv_l1       = new matrix[num_MBpart];
        for (uint8_t i = 0; i < num_MBpart; i++){matrix init(1,2,0); mv_l1[i] << init;}
        ref_idx_l0 = new int[1]();//初始化为0
        ref_idx_l1 = NULL;
        pic->get_MvNeighbour(this, 0, 0, 0, 0, mv_l0);
        mv_l0[0][0][0] += mvd_l0[0][0][0];
        mv_l0[0][0][1] += mvd_l0[0][0][1];
    }
    else
    {
        mvd_l0       = new matrix[num_MBpart];
        for (uint8_t i = 0; i < num_MBpart; i++){matrix init(1,2,0); mvd_l0[i] << init;}
        mvd_l1       = new matrix[num_MBpart];
        for (uint8_t i = 0; i < num_MBpart; i++){matrix init(1,2,0); mvd_l1[i] << init;}
            
        mv_l0       = new matrix[num_MBpart];
        for (uint8_t i = 0; i < num_MBpart; i++){matrix init(4,2,0); mv_l0[i] << init;}
        mv_l1       = new matrix[num_MBpart];
        for (uint8_t i = 0; i < num_MBpart; i++){matrix init(4,2,0); mv_l1[i] << init;}
        ref_idx_l0 = new int[4]();
        ref_idx_l1 = new int[4]();

        for(int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
            for(int subMbPartIdx = 0; subMbPartIdx < 4; subMbPartIdx++)
            {
                pic->get_MvNeighbour(this, mbPartIdx, subMbPartIdx, ref_idx_l0[mbPartIdx], 0, mv_l0);
                mv_l0[mbPartIdx][subMbPartIdx][0] += mvd_l0[mbPartIdx][0][0];
                mv_l0[mbPartIdx][subMbPartIdx][1] += mvd_l0[mbPartIdx][0][1];
                if(parser_g->debug->inter_movevector())printf(">>MoveV: (%d, %d) in B_Skip list 0\n", mv_l0[mbPartIdx][subMbPartIdx][0], mv_l0[mbPartIdx][subMbPartIdx][1]);
            }
        for(int mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
            for(int subMbPartIdx = 0; subMbPartIdx < 4; subMbPartIdx++)
            {
                pic->get_MvNeighbour(this, mbPartIdx, subMbPartIdx, ref_idx_l1[mbPartIdx], 1, mv_l1);
                mv_l1[mbPartIdx][subMbPartIdx][0] += mvd_l1[mbPartIdx][0][0];
                mv_l1[mbPartIdx][subMbPartIdx][1] += mvd_l1[mbPartIdx][0][1];
                if(parser_g->debug->inter_movevector())printf(">>MoveV: (%d, %d) in B_Skip list 1\n", mv_l1[mbPartIdx][subMbPartIdx][0], mv_l1[mbPartIdx][subMbPartIdx][1]);
            }
    }
    mb_skip_flag = 1;
    //skip也需要保持残差的结构
    Calc_residual();
}
void macroblock::Parse(int mode)
{
    mb_skip_flag = up_slice->ps->mb_skip_flag;
    type = (MbTypeName)parser_g->read_ae(21);
    mb_type = (uint8_t)type;
    //去掉枚举类型的偏移得到真实的值 

    if(type >= I_NxN && type <= I_PCM) mb_type -= 0;
    else if(type == SI_M) mb_type -= 30;
    else if(type >= P_L0_16x16 && type <= P_Skip) mb_type -= 40;
    else if(type >= B_Direct_16x16 && type <= B_Skip) mb_type -= 50;
    else std::cout << "error type" << std::endl;

    num_MBpart = Get_NumMbPart(type);
    premode = Get_MbPartPredMode(this, type, 0);
    //PCM宏块直接读取所有的数据
    if(type == I_PCM)
    {
        uint8_t* pcm_sample_luma = new uint8_t[256];
        uint8_t* pcm_sample_chroma = new uint8_t[2 * (8 * 8)];
        while(!parser_g->algi()) parser_g->read_bi();
        for(unsigned i = 0; i < 256; i++) 
            pcm_sample_luma[i] = parser_g->read_un(parser_g->pV->BitDepthY);
        for(unsigned i = 0; i < 2 * (8 * 8); i++) 
            pcm_sample_chroma[i]= parser_g->read_un(parser_g->pV->BitDepthC);
    }
    else
    {
        uint8_t noSubMbPartSizeLessThan8x8Flag = 1;
        //宏块四分，成为4个子宏块
        if(type != I_NxN && premode != Intra_16x16 && num_MBpart == 4)
        {
            //----------------------------------------------------子宏块预测

            uint8_t mbPartIdx, subMbPartIdx, compIdx, subMbPartIdexInchart_curIdx;
            uint8_t subMbPartCount;

            sub_mb_type  = new SubMbTypeName[4]{sub_type_NULL,sub_type_NULL,sub_type_NULL,sub_type_NULL};
            ref_idx_l0   = new int[4];
            ref_idx_l1   = new int[4];


            mvd_l0       = new matrix[num_MBpart];
            for (uint8_t i = 0; i < num_MBpart; i++){matrix init(4,2,0); mvd_l0[i] << init;}
            mvd_l1       = new matrix[num_MBpart];
            for (uint8_t i = 0; i < num_MBpart; i++){matrix init(4,2,0); mvd_l1[i] << init;}
                
            //步进debug这里要跳过，（会卡住
            mv_l0       = new matrix[num_MBpart];
            for (uint8_t i = 0; i < num_MBpart; i++){matrix init(4,4,0); mv_l0[i] << init;}
            mv_l1       = new matrix[num_MBpart];
            for (uint8_t i = 0; i < num_MBpart; i++){matrix init(4,4,0); mv_l1[i] << init;}

            for(mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
            {
                uint8_t sub_type = 0;
                sub_type = parser_g->read_ae(51);
                //+= 10是在枚举类型中加上B块的偏移
                if(type >= 50 && type <= 74) sub_type += 10;
                sub_mb_type[mbPartIdx] = (SubMbTypeName)sub_type;
            }

            for(mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
                if((up_slice->ps->num_ref_idx_l0_active_minus1 > 0 || up_slice->ps->mb_field_decoding_flag != up_slice->ps->field_pic_flag) &&\
                sub_mb_type[mbPartIdx] != B_Direct_8x8 && \
                type != P_8x8ref0 && \
                Get_SubMbPartPredMode(sub_mb_type[mbPartIdx]) != Pred_L1)
                    ref_idx_l0[mbPartIdx]                               = parser_g->read_ae(0x41000 + ((uint16_t)mbPartIdx << 8) + (0 << 4) + 0);
                else ref_idx_l0[mbPartIdx] = 0;
            for(mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
                if((up_slice->ps->num_ref_idx_l1_active_minus1 > 0 || up_slice->ps->mb_field_decoding_flag != up_slice->ps->field_pic_flag) &&\
                sub_mb_type[mbPartIdx] != B_Direct_8x8 &&\
                Get_SubMbPartPredMode(sub_mb_type[mbPartIdx]) != Pred_L0)
                    ref_idx_l1[mbPartIdx]                               = parser_g->read_ae(0x41000 + ((uint16_t)mbPartIdx << 8) + (0 << 4) + 1);
                else ref_idx_l1[mbPartIdx] = 0;
            for(mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
            {
                if(sub_mb_type[mbPartIdx] != B_Direct_8x8 && Get_SubMbPartPredMode(sub_mb_type[mbPartIdx]) != Pred_L1) 
                    for(subMbPartIdx = 0; subMbPartIdx < (subMbPartCount = subMbInfo[subMbPartIdexInchart_curIdx][1]); subMbPartIdx++)
                    {
                        for(compIdx = 0; compIdx < 2; compIdx++)
                        {
                            mvd_l0[mbPartIdx][subMbPartIdx][compIdx] =  parser_g->read_ae(0x43000 + ((uint16_t)mbPartIdx << 8) + (subMbPartIdx << 4)  + (compIdx << 1) + 0);
                        }
                        pic->get_MvNeighbour(this, mbPartIdx, subMbPartIdx, ref_idx_l0[mbPartIdx], 0, mv_l0);
                                                if(parser_g->debug->inter_movevector())
                        {
                            printf(">>mvd_l0: (%d, %d) in 4x4sub\n", mvd_l0[mbPartIdx][subMbPartIdx][0], mvd_l0[mbPartIdx][subMbPartIdx][1]);
                            printf(">>mv_l0 : (%d, %d) in 4x4sub\n", mv_l0[mbPartIdx][subMbPartIdx][0], mv_l0[mbPartIdx][subMbPartIdx][1]);
                        }
                        mv_l0[mbPartIdx][subMbPartIdx][0] += mvd_l0[mbPartIdx][subMbPartIdx][0];
                        mv_l0[mbPartIdx][subMbPartIdx][1] += mvd_l0[mbPartIdx][subMbPartIdx][1];
                    }
            }
            for(mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
            {
                if(sub_mb_type[mbPartIdx] != B_Direct_8x8 && Get_SubMbPartPredMode(sub_mb_type[mbPartIdx]) != Pred_L0)
                    for(subMbPartIdx = 0; subMbPartIdx < (subMbPartCount = subMbInfo[subMbPartIdexInchart_curIdx][1]); subMbPartIdx++)
                    {
                        
                        for(compIdx = 0; compIdx < 2; compIdx++) 
                        {
                            mvd_l1[mbPartIdx][subMbPartIdx][compIdx] =  parser_g->read_ae(0x43000 + ((uint16_t)mbPartIdx << 8) + (subMbPartIdx << 4)  + (compIdx << 1) + 1);
                        }
                        if(parser_g->debug->inter_movevector())
                        {
                            printf(">>mvd_l1: (%d, %d) in 4x4sub\n", mvd_l1[mbPartIdx][subMbPartIdx][0], mvd_l1[mbPartIdx][subMbPartIdx][1]);
                            printf(">>mv_l1 : (%d, %d) in 4x4sub\n", mv_l1[mbPartIdx][subMbPartIdx][0], mv_l1[mbPartIdx][subMbPartIdx][1]);
                        }
                        mv_l1[mbPartIdx][subMbPartIdx][0] += mvd_l1[mbPartIdx][subMbPartIdx][0];
                        mv_l1[mbPartIdx][subMbPartIdx][1] += mvd_l1[mbPartIdx][subMbPartIdx][1];
                        pic->get_MvNeighbour(this, mbPartIdx, subMbPartIdx, ref_idx_l1[mbPartIdx], 1, mv_l1);
                    }
                else //B_direct_8x8预测 
                {

                }
            }
            //--------------------------------------------------------子宏块预测结束
            for(uint8_t mbPartIdx = 0; mbPartIdx < 4; mbPartIdx++)
            {
                if(sub_mb_type[mbPartIdx] != B_Direct_8x8)
                { 
                    if(subMbInfo[sub_mb_type[mbPartIdx]][1] > 1) 
                        noSubMbPartSizeLessThan8x8Flag = 0;
                }
                else if(!parser_g->pS->sps->direct_8x8_inference_flag) noSubMbPartSizeLessThan8x8Flag = 0;
            }
        }
        //宏块小于4分（2分或者不分）
        else if(num_MBpart <= 2)
        {
            //if is 8x8 transform decode  and  is I_NxN
            if(parser_g->pS->pps->transform_8x8_mode_flag && type == I_NxN)
                transform_size_8x8_flag = parser_g->read_ae(22);
            if(type == I_NxN)
            {
                if(transform_size_8x8_flag) premode = Intra_8x8;
                else premode = Intra_4x4;
            }
            uint8_t luma4x4BlkIdx, luma8x8BlkIdx, compIdx;
            //帧内预测
            if(premode == Intra_4x4 || premode == Intra_8x8 || premode== Intra_16x16)
            { 
                if(premode == Intra_4x4)
                {
                    rem_intra4x4_pred_mode = new uint8_t[16];
                    prev_intra4x4_pred_mode_flag = new uint8_t[16];
                    for(luma4x4BlkIdx=0; luma4x4BlkIdx<16; luma4x4BlkIdx++) 
                    { 
                        prev_intra4x4_pred_mode_flag[luma4x4BlkIdx] = parser_g->read_ae(31);
                        if(!prev_intra4x4_pred_mode_flag[luma4x4BlkIdx]) 
                            rem_intra4x4_pred_mode[luma4x4BlkIdx] = parser_g->read_ae(32);
                    }
                    
                }
                // if(Get_MbPartPredMode(mb_type, 0) == Intra_8x8)
                // {
                //     uint8_t* prev_intra8x8_pred_mode_flag = new uint8_t[4];
                //     uint8_t* rem_intra8x8_pred_mode = new uint8_t[4];
                //     for(luma8x8BlkIdx=0; luma8x8BlkIdx < 4; luma8x8BlkIdx++)
                //     { 
                //         prev_intra8x8_pred_mode_flag[luma8x8BlkIdx];
                //         if(!prev_intra8x8_pred_mode_flag[luma8x8BlkIdx]){
                //             rem_intra8x8_pred_mode[luma8x8BlkIdx];
                //         }
                //     } 
                // }
                //16x16的帧内预测，一共四种方式
                if(parser_g->pV->ChromaArrayType == 1 || parser_g->pV->ChromaArrayType == 2) 
                    intra_chroma_pred_mode = parser_g->read_ae(35);//ue|ae
            }
            //帧间预测
            else if(premode != Direct)
            {
                ref_idx_l0   = new int[4];
                ref_idx_l1   = new int[4];

                mvd_l0       = new matrix[num_MBpart];
                for (uint8_t i = 0; i < num_MBpart; i++){matrix init(1,2,0); mvd_l0[i] << init;}
                mvd_l1       = new matrix[num_MBpart];
                for (uint8_t i = 0; i < num_MBpart; i++){matrix init(1,2,0); mvd_l1[i] << init;}

                mv_l0       = new matrix[num_MBpart];
                for (uint8_t i = 0; i < num_MBpart; i++){matrix init(1,2,0); mv_l0[i] << init;}
                mv_l1       = new matrix[num_MBpart];
                for (uint8_t i = 0; i < num_MBpart; i++){matrix init(1,2,0); mv_l1[i] << init;}

                uint8_t mbPartIdx;
                for(mbPartIdx = 0; mbPartIdx < num_MBpart; mbPartIdx++) 
                    if((up_slice->ps->num_ref_idx_l0_active_minus1 > 0 || \
                    up_slice->ps->mb_field_decoding_flag != up_slice->ps->field_pic_flag) && \
                    Get_MbPartPredMode(this, type, mbPartIdx) != Pred_L1)
                        ref_idx_l0[mbPartIdx]                      = parser_g->read_ae(0x41000 + ((uint16_t)mbPartIdx << 8) + (0 << 4) + 0); 
                    else ref_idx_l0[mbPartIdx] = 0;
                for(mbPartIdx = 0; mbPartIdx < num_MBpart; mbPartIdx++) 
                    if((up_slice->ps->num_ref_idx_l1_active_minus1 > 0 || \
                    up_slice->ps->mb_field_decoding_flag != up_slice->ps->field_pic_flag) && 
                    Get_MbPartPredMode(this, type, mbPartIdx) != Pred_L0)
                        ref_idx_l1[mbPartIdx]                      = parser_g->read_ae(0x41000 + ((uint16_t)mbPartIdx << 8) + (0 << 4) + 1);
                    else ref_idx_l1[mbPartIdx] = 0;
                for(mbPartIdx = 0; mbPartIdx < num_MBpart; mbPartIdx++)
                {
                    premode = Get_MbPartPredMode(this, type, mbPartIdx);
                    if(premode != Pred_L1 && premode != Pred_NU)
                    {
                        for(compIdx = 0; compIdx < 2; compIdx++)
                        {
                            mvd_l0[mbPartIdx][0][compIdx]          = parser_g->read_ae(0x43000 + ((uint16_t)mbPartIdx << 8) + (0 << 4)  + (compIdx << 1) + 0); 
                        }
                        pic->get_MvNeighbour(this, mbPartIdx, 0, ref_idx_l0[mbPartIdx], 0,mv_l0);
                        if(parser_g->debug->inter_movevector())
                        {
                            printf(">>mvd_l0: (%d, %d)\n", mvd_l0[mbPartIdx][0][0], mvd_l0[mbPartIdx][0][1]);
                            printf(">>mv_l0 : (%d, %d)\n", mv_l0[mbPartIdx][0][0], mv_l0[mbPartIdx][0][1]);
                        }
                        mv_l0[mbPartIdx][0][0] += mvd_l0[mbPartIdx][0][0];
                        mv_l0[mbPartIdx][0][1] += mvd_l0[mbPartIdx][0][1];
                    }
                }
                for(mbPartIdx = 0; mbPartIdx < num_MBpart; mbPartIdx++)
                {
                    premode = Get_MbPartPredMode(this, type, mbPartIdx);
                    if(premode != Pred_L0 && premode != Pred_NU)
                    {
                        for(compIdx = 0; compIdx < 2; compIdx++)
                        {
                            mvd_l1[mbPartIdx][0][compIdx]          = parser_g->read_ae(0x43000 + ((uint16_t)mbPartIdx << 8) + (0 << 4)  + (compIdx << 1) + 1);
                        }
                        pic->get_MvNeighbour(this, mbPartIdx, 0, ref_idx_l1[mbPartIdx], 1, mv_l1);
                        if(parser_g->debug->inter_movevector())
                        {
                            printf(">>mvd_l1: (%d, %d)\n", mvd_l1[mbPartIdx][0][0], mvd_l1[mbPartIdx][0][1]);
                            printf(">>mv_l1 : (%d, %d)\n", mv_l1[mbPartIdx][0][0], mv_l1[mbPartIdx][0][1]);
                        }
                        mv_l1[mbPartIdx][0][0] += mvd_l1[mbPartIdx][0][0];
                        mv_l1[mbPartIdx][0][1] += mvd_l1[mbPartIdx][0][1];
                    }
                }
            }
            //----------------------------------------------------------
        }
        else //Direct预测 指的是 B_Direct_16x16
        {
            ref_idx_l0   = new int[1]();
            ref_idx_l1   = new int[1]();

            mvd_l0       = new matrix[4];
            for (uint8_t i = 0; i < 4; i++){matrix init(1,2,0); mvd_l0[i] << init;}
            mvd_l1       = new matrix[4];
            for (uint8_t i = 0; i < 4; i++){matrix init(1,2,0); mvd_l1[i] << init;}
                
            //步进debug这里要跳过，（会卡住
            mv_l0       = new matrix[4];
            for (uint8_t i = 0; i < 4; i++){matrix init(4,4,0); mv_l0[i] << init;}
            mv_l1       = new matrix[4];
            for (uint8_t i = 0; i < 4; i++){matrix init(4,4,0); mv_l1[i] << init;}
            
            //

        }
        //色彩编码模式
        //非帧内16x16的色彩系数编码模式由读取得到，
        if(premode != Intra_16x16)
        {
            coded_block_pattern = parser_g->read_ae(23);
            CodedBlockPatternLuma = coded_block_pattern % 16;
            CodedBlockPatternChroma = coded_block_pattern / 16;
            
            if(CodedBlockPatternLuma > 0 &&\
            parser_g->pS->pps->transform_8x8_mode_flag && type != I_NxN &&\
            noSubMbPartSizeLessThan8x8Flag &&\
            (type != B_Direct_16x16 || parser_g->pS->sps->direct_8x8_inference_flag)) 
            {
                transform_size_8x8_flag = parser_g->read_ae(22);
            }
        }
        //帧内16x16的色彩系数编码模式由查表得到，
        else
        {
            CodedBlockPatternChroma = macroBlockInfo_I_slice[mb_type][3];
            CodedBlockPatternLuma = macroBlockInfo_I_slice[mb_type][4];
        }
        //计算残差
        Calc_residual();
    }
}
void macroblock::Calc_residual()
{
    //残差解析和解码
    re = new residual(this, parser_g);
    if(CodedBlockPatternLuma > 0 || CodedBlockPatternChroma > 0 || premode == Intra_16x16)
    {
        //先计算好量化参数
        mb_qp_delta  = parser_g->read_ae(25);
        QPY = ((QPY_prev + mb_qp_delta + 52 + 2 * parser_g->pV->QpBdOffsetY) % (52 + parser_g->pV->QpBdOffsetY)) - parser_g->pV->QpBdOffsetY;
        QPY_ = QPY + parser_g->pV->QpBdOffsetY;

        QPC_ = 0;
        QPC_ = QPY_ > 2 ? (QPY_ - 2): QPC_;
        if(parser_g->pS->sps->qpprime_y_zero_transform_bypass_flag == 1 && QPY_ == 0)  TransformBypassModeFlag = 1;
        else  TransformBypassModeFlag = 0;
        
        //然后解残差
        re->Parse(0);
        re->Decode(0);
    }
    else//不编码全部置为0；这里需要保持block的结构（coded_block_flag的推导需要），但是不需要数据
    {
        //不编码的时候仍然需要量化参数，只是不需要cabac读取了，
        //mb_qp_delta = 0;
        QPY = ((QPY_prev + 0 + 52 + 2 * parser_g->pV->QpBdOffsetY) % (52 + parser_g->pV->QpBdOffsetY)) - parser_g->pV->QpBdOffsetY;
        QPY_ = QPY + parser_g->pV->QpBdOffsetY;

        QPC_ = 0;
        QPC_ = QPY_ > 2 ? (QPY_ - 2): QPC_;

        //全部置零，生成相应的空Block
        re->ZeroAll();
    }
    //解完残差把残差挂到宏块上来，用起来直观一点
    this->residual_Y = re->residual_Y;    
    this->residual_U = re->residual_U;    
    this->residual_V = re->residual_V;
}

void macroblock::Weight_defaultWeight(matrix& m0, matrix& m1, matrix& out, bool flag_0, bool flag_1)
{
    matrix zero(out.x_length, out.y_length, 0);
    if(flag_0 && flag_1)
    {out += (m0 + m1);}
    else
    {out += ((flag_0?m0:zero) + (flag_1?m1:zero));}
}
void macroblock::Weight_CoefficWeight(bool is_explicit, matrix& m0, matrix& m1, matrix& out, int refidx_l0, int refidx_l1, bool flag_0, bool flag_1)
{
    int BitDepth_Y = up_slice->ps->sps->bit_depth_luma_minus8 + 8;
    int logWD_C= 5;
    int W_0C   = 0;
    int W_1C   = 0;
    int O_0C   = 32;
    int O_1C   = 32;
    if(is_explicit)
    {
        logWD_C= up_slice->pw->luma_log2_weight_denom;
        W_0C   = up_slice->pw->luma_weight_l0[refidx_l0];
        O_0C   = up_slice->pw->luma_offset_l0[refidx_l0] * (1 << (BitDepth_Y - 8));
        if(flag_1)
        {
            W_1C   = up_slice->pw->luma_weight_l1[refidx_l1];
            O_1C   = up_slice->pw->luma_offset_l1[refidx_l1] * (1 << (BitDepth_Y - 8));
        }
    }
    if(flag_0 && !flag_1)
    {
        for (uint8_t r = 0; r < out.x_length ; r++)
        {
            for (uint8_t c = 0; c < out.y_length; c++)
            {
                if(logWD_C >= 1) 
                    out[r][c] = Clip1Y(   ((int)((m0[r][c] * W_0C) + powl(2, logWD_C - 1)) >> logWD_C) + O_0C, BitDepth_Y);
                else out[r][c] = Clip1Y(m0[r][c] * W_0C + O_0C, BitDepth_Y);
            }
        }
    }
    else if(!flag_0 && flag_1)
    {
        for (uint8_t r = 0; r < out.x_length ; r++)
        {
            for (uint8_t c = 0; c < out.y_length; c++)
            {
                if(logWD_C >= 1)
                    out[r][c] = Clip1Y(   ((int)((m1[r][c] * W_1C) + powl(2, logWD_C - 1)) >> logWD_C) + O_1C, BitDepth_Y);
                else out[r][c] = Clip1Y(m0[r][c] * W_1C + O_1C, BitDepth_Y);
            }
        }
    }
    else
    {
        for (uint8_t r = 0; r < out.x_length ; r++)
        {
            for (uint8_t c = 0; c < out.y_length; c++)
            {
                out[r][c] = Clip1Y(   ((int)((m0[r][c] * W_0C) + ((m1[r][c] * W_1C)) + powl(2, logWD_C)) >> (logWD_C + 1)) + ((O_1C + O_0C + 1) >> 1), BitDepth_Y);
            }
        }
    }
}

void macroblock::Decode(int index)
{
    if(this->is_intrapred())
    {
        switch (premode)
        {
        case Intra_16x16:Prediction_Intra16x16(); break;
        case Intra_4x4  :Prediction_Intra4x4(index);break;
        case Intra_8x8  :Prediction_Intra8x8();break;
        default:printf("undef intra decode way");break;
        }
    }
    else //if(this->is_interpred())
    {
        //如果不是16x16直接预测
        if(up_slice->get_type() != B && type != B_Direct_16x16 && type != B_Skip)
        {
            
            //获取宏块的子块数量
            uint8_t mbPart = Get_NumMbPart(type);
            uint8_t width = Get_MbPartWidth(type);
            uint8_t height = Get_MbPartHeight(type);

            for (uint8_t mbPartIdx = 0; mbPartIdx < mbPart; mbPartIdx++)
            {
                int refidx_l0 = ref_idx_l0?ref_idx_l0[mbPartIdx]:0;
                int refidx_l1 = ref_idx_l1?ref_idx_l1[mbPartIdx]:0;
                bool predFlag_0 = Get_PredFlag(this, mbPartIdx, 0);
                bool predFlag_1 = Get_PredFlag(this, mbPartIdx, 1);
                //如果有子子块，那么循环的最大值增加
                uint8_t subMbPart = num_MBpart == 4 ? Get_SubNumMbPart(sub_mb_type[mbPartIdx]) : 1;
                for (uint8_t subPartIdx = 0; subPartIdx < subMbPart; subPartIdx++)
                {
                    //如果是4分块，那么把宽高都更新为子子块的宽和高
                    if(num_MBpart == 4)
                    {
                        width  = Get_SubMbPartWidth(sub_mb_type[mbPartIdx]);
                        height = Get_SubMbPartHeight(sub_mb_type[mbPartIdx]);
                    }
                    matrix tmp_0(height, width, 0), tmp_1(height, width, 0);
                    //如果有前向预测，
                    if(predFlag_0)
                    {
                        
                        picture* ref_pic_0 = up_slice->decoder->get_Ref0PicByI(0);
                        if(parser_g->debug->pic_terminalchar())
                        // cout << (*ref_pic_0) << endl;
                        Prediction_Inter(tmp_0 ,mbPartIdx, subPartIdx, width, height, ref_pic_0, mv_l0,1);
                        
                    }
                    //如果有后向预测
                    if(predFlag_1)
                    {
                        picture* ref_pic_0 = up_slice->decoder->get_Ref1PicByI(0);
                        Prediction_Inter(tmp_1 ,mbPartIdx, subPartIdx, width, height, ref_pic_0, mv_l1,1);
                    }
                    //加权
                    uint8_t weighted_pred_flag = up_slice->ps->pps->weighted_pred_flag;
                    uint8_t weighted_bipred_idc = up_slice->ps->pps->weighted_bipred_idc;

                    matrix out(height, width, 0);
                    Slicetype sltype = up_slice->get_type();
                    if(sltype == P || sltype == SP)
                    {
                        if(weighted_pred_flag) 
                        {
                            //显式加权
                            Weight_CoefficWeight(true, tmp_0, tmp_1, out, refidx_l0, refidx_l1, predFlag_0, predFlag_1);
                        }
                        else
                        {
                            //默认加权
                            Weight_defaultWeight(tmp_0, tmp_1, out, predFlag_0, predFlag_1);
                        }
                        
                    }
                    else //if(sltype == B)
                    {
                        if(weighted_bipred_idc == 0)
                        {
                            //默认加权
                            Weight_defaultWeight(tmp_0, tmp_1, out, predFlag_0, predFlag_1);
                        }
                        else if(weighted_bipred_idc == 1)
                        {
                            //显式加权
                            Weight_CoefficWeight(true, tmp_0, tmp_1, out, refidx_l0, refidx_l1, predFlag_0, predFlag_1);
                        }
                        //B片的额外的加权预测
                        else //if(weighted_bipred_idc == 2)
                        {
                            if(predFlag_1 && predFlag_0)
                            {
                                //隐式加权
                                Weight_CoefficWeight(false, tmp_0, tmp_1, out, refidx_l0, refidx_l1, predFlag_0, predFlag_1);
                            }
                            else
                            {
                                //默认加权
                                Weight_defaultWeight(tmp_0, tmp_1, out, predFlag_0, predFlag_1);
                            }
                        }

                    }
                    //赋值
                    int mb_partWidth = Get_MbPartWidth(type);
                    int mb_partHeigh = Get_MbPartHeight(type);
                    int xS = (mbPartIdx % (16 / mb_partWidth)) * mb_partWidth +\
                                                                                (num_MBpart == 4 ? ((subPartIdx * Get_SubMbPartWidth(sub_mb_type[mbPartIdx])) % 8) : 0);
                    int yS = (mbPartIdx / (16 / mb_partWidth)) * mb_partHeigh +\
                                                                                (num_MBpart == 4 ? ((subPartIdx * Get_SubMbPartWidth(sub_mb_type[mbPartIdx])) / 8) : 0);
                    
                    for(uint8_t yL = 0; yL < height; yL++)
                    {
                        for(uint8_t xL = 0; xL < width; xL++)
                        {
                            //到预测样点矩阵中，
                            (*pred_Y)[yS + yL][xS + xL] += out[yL][xL];
                        }
                    }
                }
            }
        }
        else
        {

        }
        
    }
}
void macroblock::Prediction_Inter(matrix& out, uint8_t mbPartIdx, uint8_t subMbPartIdx, uint8_t width_part, uint8_t height_part, picture* ref_pic, matrix* mv_lx, bool predFlag)
{
    int mvLX[2] = {mv_lx[mbPartIdx][subMbPartIdx][0], mv_lx[mbPartIdx][subMbPartIdx][1]};
    //xAL 是子块、子子块的图片中的绝对起始坐标
    int xAL = position_y * 16, xBL = 0;
    int yAL = position_x * 16, yBL = 0;
    //xL yL 是在块中的坐标
    int xL = 0, yL = 0, xFracL = 0, yFracL = 0;
    int xIntL = 0, yIntL = 0;
    //得到子块的起始绝对坐标

    int mb_partWidth = Get_MbPartWidth(type);
    int mb_partHeigh = Get_MbPartHeight(type);

    xAL += (mbPartIdx % (16 / mb_partWidth)) * mb_partWidth +\
                                                                (num_MBpart == 4 ? ((subMbPartIdx * Get_SubMbPartWidth(sub_mb_type[mbPartIdx])) % 8) : 0);
    yAL += (mbPartIdx / (16 / mb_partWidth)) * mb_partHeigh +\
                                                                (num_MBpart == 4 ? ((subMbPartIdx * Get_SubMbPartWidth(sub_mb_type[mbPartIdx])) / 8) : 0);
    //下面这步是直接求并和移位（负数也是，不需要处理）
    xFracL = (mvLX[0] & 3); yFracL = /*Sig(mvLX[1]) * */ (mvLX[1] & 3);
    xBL = xAL + (mvLX[0] >> 2); yBL = yAL +  /*Sig(mvLX[1]) **/ (mvLX[1] >> 2);
    //每一个样点都需要内插
    for(uint8_t yL = 0; yL < height_part; yL++)
    {
        for(uint8_t xL = 0; xL < width_part; xL++)
        {
            //加上运动矢量的整数坐标
            //对应在参考图片中的起始坐标加上块内的坐标分别得到整数绝对坐标
            xIntL = xBL + xL;
            yIntL = yBL + yL;
            //前向参考直接加到预测样点矩阵中，
            out[yL][xL] += Prediction_Inter_LumaSampleInterpolation(xIntL, yIntL, xFracL, yFracL, ref_pic);
        }
    }
}

//分数像素内插，大写字母都是整数像素，小写字母都是分数像素
int macroblock::Prediction_Inter_LumaSampleInterpolation(int xIntL, int yIntL, int xFracL, int yFracL, picture* ref_pic)
{
    int BitDepthY = parser_g->pV->BitDepthY;
    int A = 0, B = 0, C = 0, D = 0, E = 0, F = 0, G = 0, H = 0, I = 0, J = 0,\
        K = 0, L = 0, M = 0, N = 0, P = 0, Q = 0, R = 0, S = 0, T = 0, U = 0;
    int* sampleX[20] = {&A,&B,&C,&D,&E,&F,&G,&H,&I,&J,&K,&L,&M,&N,&P,&Q,&R,&S,&T,&U};
    int PicWidthInSample = parser_g->pV->PicWidthInMbs * 16;
    int PicHeightInSample = parser_g->pV->PicHeightInMbs * 16;
    auto func = [xIntL, yIntL, ref_pic, PicWidthInSample, PicHeightInSample](int xDZL, int yDZL)->int{
        int sample = 0;
        int xZL = 0, yZL = 0;

        xZL = Clip3(0, PicWidthInSample -1, xIntL + xDZL);
        yZL = Clip3(0, PicHeightInSample -1, yIntL + yDZL);
        sample = ref_pic->get_SampleXY(xZL, yZL, 0);

        return sample;
    };
    //如果分数都是0，那么直接返回G样点，不做后续的计算
    G = func(xyDZL[6][0], xyDZL[6][1]);
    if(xFracL == 0 && yFracL == 0) return G;

    for (uint8_t i = 0; i < 20; i++)
    {
        *(sampleX[i]) = func(xyDZL[i][0], xyDZL[i][1]);
    }

    int A1 = func(-2,-2), A0 = func(-1, -2), B0 = func(2, -2), B1 = func(3, -2);
    int C1 = func(-2,-2), C0 = func(-1, -2), D0 = func(2, -2), D1 = func(3, -2);
    int R1 = func(-2, 2), R0 = func(-1,  2), S0 = func(2, -2), S1 = func(3, -2);
    int T1 = func(-2, 3), T0 = func(-1,  3), U0 = func(2, -3), U1 = func(3, -3);

    // printf(">>Inter: insert sample:\n");
    // printf("        int pos :xIntL  yIntL  (%3d, %3d)\n", xIntL, yIntL);
    // printf("        fra pos :xFracL yFracL (%3d, %3d)\n", xFracL, yFracL);
    // printf("        %d, %d,  %d, %d, %d, %d\n", A1, A0, A, B, B0, B1);
    // printf("        %d, %d,  %d, %d, %d, %d\n", C1, C0, C, D, D0, D1);
    // printf("        %d, %d, [%d, %d, %d, %d]\n", E, F, G, H, I, J);
    // printf("        %d, %d, [%d, %d, %d, %d]\n", K, L, M, N, P, Q);
    // printf("        %d, %d, [%d, %d, %d, %d]\n", R1, R0, R, S, S0, S1);
    // printf("        %d, %d, [%d, %d, %d, %d]\n", T1, T0, T, U, U0, U1);
    // printf("==Inter: \n");


    int aa = SixTapFliter(A1, A0, A, B, B0, B1);
    int bb = SixTapFliter(C1, C0, C, D, D0, D1);
    int gg = SixTapFliter(R1, R0, R, S, S0, S1);
    int hh = SixTapFliter(T1, T0, T, U, U0, U1);

    int b1 = SixTapFliter(E, F, G, H, I, J);
    int h1 = SixTapFliter(A, C, G, M, R, T);
    int s1 = SixTapFliter(K, L, M, N, P, Q);
    int m1 = SixTapFliter(B, D, H, N, S, U);
    int b = Clip1Y((b1 + 16) >> 5, BitDepthY);
    int h = Clip1Y((h1 + 16) >> 5, BitDepthY);


    int j1 = SixTapFliter(aa, bb, b1, s1, gg, hh);
    int j = Clip1Y((j1 + 512) >> 10, BitDepthY);

    int s = Clip1Y((s1 + 16) >> 5, BitDepthY);
    int m = Clip1Y((m1 + 16) >> 5, BitDepthY);

    int a = (G + b + 1) >> 1;
    int c = (H + b + 1) >> 1;
    int d = (G + h + 1) >> 1;
    int n = (M + h + 1) >> 1;
    int f = (b + j + 1) >> 1;
    int i = (h + j + 1) >> 1;
    int k = (j + m + 1) >> 1;
    int q = (j + s + 1) >> 1;

    int e = (b + h + 1) >> 1;
    int g = (b + m + 1) >> 1;
    int p = (h + s + 1) >> 1;
    int r = (m + s + 1) >> 1;

    int* predPart[4][4] = 
    {
        {&G, &d, &h, &n},
        {&a, &e, &i, &p},
        {&b, &f, &j, &q},
        {&c, &g, &k, &r}
    };
    // printf(">>Inter: result: %d\n", *(predPart[xFracL][yFracL]));
    return *(predPart[xFracL][yFracL]);
}
void macroblock::Prediction_Intra4x4(int index)
{
    int luma4x4Index = block4x4Index[index];
    //如果是第一个4x4块，那么先解析整个宏块的预测模式
    if(index == 0) ParseIntra4x4PredMode();

    matrix pred_I(4,4,0);
    ParseIntra4x4(pred_I, Intra4x4PredMode[index], index);

    int luma4x4PosR = (luma4x4Index/4) * 4, luma4x4PosC = (luma4x4Index%4) * 4;
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            (*pred_Y)[luma4x4PosR + i][luma4x4PosC + j] += pred_I[i][j];
        }
    }
}

void macroblock::ParseIntra4x4PredMode()
{
    int  luma4x4BlkIdxA,  luma4x4BlkIdxB;
    uint8_t  intraMxMPredModeA,  intraMxMPredModeB;
    macroblock* A, * B;
    uint8_t  dcPredModePredictedFlag  = 0;
    Intra4x4PredMode = new uint8_t[16];
    for (uint8_t i = 0; i < 16; i++)
    {
        luma4x4BlkIdxA = 0;
        A = pic->get_BLneighbour(this, 'A', i, 0x011, luma4x4BlkIdxA);
        luma4x4BlkIdxB = 0;
        B = pic->get_BLneighbour(this, 'B', i, 0x011, luma4x4BlkIdxB);
        if(!this->is_avaiable(A) || !this->is_avaiable(B) ||\
           (A->is_interpred() && 1) ||\
           (B->is_interpred() && 1))
            dcPredModePredictedFlag = 1;
        else dcPredModePredictedFlag = 0;

        if(dcPredModePredictedFlag == 1 || (A->premode != Intra_8x8 && A->premode != Intra_4x4)) intraMxMPredModeA = 2;
        else 
        {
            if(A->premode == Intra_4x4) intraMxMPredModeA = A->Intra4x4PredMode[luma4x4BlkIdxA];
            else  intraMxMPredModeA = A->Intra8x8PredMode[luma4x4BlkIdxA >> 2];
        }
        if(dcPredModePredictedFlag == 1 || (B->premode != Intra_8x8 && B->premode != Intra_4x4)) intraMxMPredModeB = 2;
        else 
        {
            if(B->premode == Intra_4x4) intraMxMPredModeB = B->Intra4x4PredMode[luma4x4BlkIdxB];
            else  intraMxMPredModeB = B->Intra8x8PredMode[luma4x4BlkIdxB >> 2];
        }

        uint8_t predIntra4x4PredMode;
        predIntra4x4PredMode = Min(intraMxMPredModeA, intraMxMPredModeB);
        if(prev_intra4x4_pred_mode_flag[i]) Intra4x4PredMode[i] = predIntra4x4PredMode;
        else 
        {
            if(rem_intra4x4_pred_mode[i] < predIntra4x4PredMode) Intra4x4PredMode[i] = rem_intra4x4_pred_mode[i];
            else Intra4x4PredMode[i] = rem_intra4x4_pred_mode[i] + 1;
        }
    }
    Sdelete_l(rem_intra4x4_pred_mode);
    Sdelete_l(prev_intra4x4_pred_mode_flag);
}

void macroblock::ParseIntra4x4(matrix& predI, int mode, int index)
{
    switch (mode)
    {
    case 0:Prediction_Intra4x4_V(predI,index); break;
    case 1:Prediction_Intra4x4_H(predI,index); break;
    case 2:Prediction_Intra4x4_DC(predI,index); break;
    case 3:Prediction_Intra4x4_Diagonal_Down_Left(predI,index); break;
    case 4:Prediction_Intra4x4_Diagonal_Down_Right(predI,index); break;
    case 5:Prediction_Intra4x4_V_Right(predI,index);break;
    case 6:Prediction_Intra4x4_H_Down(predI,index);break;
    case 7:Prediction_Intra4x4_V_Left(predI,index); break;
    case 8:Prediction_Intra4x4_H_Up(predI,index); break;
    default:std::cout << "nopred mode for 4x4" << std::endl;break;
    }
}

matrix& macroblock::get_4x4Neighbour(matrix& sample, int index)
{
    int predIndex = 0;

    int16_t I = -1; 
    int16_t J = -1; 
    int16_t K = -1; 
    int16_t L = -1;
    int r_A = 0, c_A = 0;
    macroblock* mbAddrA = pic->get_BLneighbour(this, 'A', index, 0x011, predIndex, r_A, c_A);
    if(mbAddrA)
    {
    I = mbAddrA->sample_Y->get_value_xy(r_A + 0, c_A);
    J = mbAddrA->sample_Y->get_value_xy(r_A + 1, c_A);
    K = mbAddrA->sample_Y->get_value_xy(r_A + 2, c_A);
    L = mbAddrA->sample_Y->get_value_xy(r_A + 3, c_A);
    }

    int16_t A = -1; 
    int16_t B = -1; 
    int16_t C = -1; 
    int16_t D = -1;
    int r_B = 0, c_B = 0;
    macroblock* mbAddrB = pic->get_BLneighbour(this, 'B', index, 0x011, predIndex, r_B, c_B);
    if(mbAddrB)
    {
    A = mbAddrB->sample_Y->get_value_xy(r_B, c_B + 0);
    B = mbAddrB->sample_Y->get_value_xy(r_B, c_B + 1);
    C = mbAddrB->sample_Y->get_value_xy(r_B, c_B + 2);
    D = mbAddrB->sample_Y->get_value_xy(r_B, c_B + 3);
    }

    int16_t E = -1; 
    int16_t F = -1; 
    int16_t G = -1; 
    int16_t H = -1;
    int r_C = 0, c_C = 0;
    macroblock* mbAddrC = pic->get_BLneighbour(this, 'C', index, 0x011, predIndex, r_C, c_C);
    if(this->is_avaiable(mbAddrC))
    {
        if(index == 3 || index == 11)
        {
            E = F = G = H = D;
        }
        else
        {
            E = mbAddrC->sample_Y->get_value_xy(r_C, c_C + 0);
            F = mbAddrC->sample_Y->get_value_xy(r_C, c_C + 1);
            G = mbAddrC->sample_Y->get_value_xy(r_C, c_C + 2);
            H = mbAddrC->sample_Y->get_value_xy(r_C, c_C + 3);
        }
        
    }
    else {E = F = G = H = D;}

    if((E == -1 && F == -1 && G == -1 && H == -1) && D != -1)
    {E = F = G = H = D;}

    int16_t M = 0;
    int r_D = 0, c_D = 0;
    macroblock* mbAddrD = pic->get_BLneighbour(this, 'D', index, 0x011, predIndex, r_D, c_D);
    if(mbAddrD)
    M = mbAddrD->sample_Y->get_value_xy(r_D, c_D);
    
    sample[1][0] = I; sample[2][0] = J; sample[3][0] = K; sample[4][0] = L;
    sample[0][1] = A; sample[0][2] = B; sample[0][3] = C; sample[0][4] = D;
    sample[0][5] = E; sample[0][6] = F; sample[0][7] = G; sample[0][8] = H;
    sample[0][0] = M;

    return sample;
}
void macroblock::Prediction_Intra4x4_V                   (matrix& pred, int index)
{    int predIndex = 0;

    int16_t A = 0, B = 0, C = 0, D = 0;
    int r_B = 0, c_B = 0;
    macroblock* mbAddrB = pic->get_BLneighbour(this, 'B', index, 0x011, predIndex, r_B, c_B);
    if(mbAddrB)
    {
        A = mbAddrB->sample_Y->get_value_xy(r_B, c_B + 0);
        B = mbAddrB->sample_Y->get_value_xy(r_B, c_B + 1);
        C = mbAddrB->sample_Y->get_value_xy(r_B, c_B + 2);
        D = mbAddrB->sample_Y->get_value_xy(r_B, c_B + 3);
    }

    matrix p(5,9,0);
    p[0][1] = A; p[0][2] = B; p[0][3] = C; p[0][4] = D;
    
    for (uint8_t i = 0; i < 4; i++)
    {
        pred.Set_c(i, p[0][i + 1]);
    }
    
}
void macroblock::Prediction_Intra4x4_H                   (matrix& pred, int index)
{
    int predIndex = 0;

    int16_t I = 0, J = 0, K = 0, L = 0;
    int r_A = 0, c_A = 0;
    macroblock* mbAddrA = pic->get_BLneighbour(this, 'A', index, 0x011, predIndex, r_A, c_A);
    if(this->is_avaiable(mbAddrA))
    {
        I = mbAddrA->sample_Y->get_value_xy(r_A + 0, c_A);
        J = mbAddrA->sample_Y->get_value_xy(r_A + 1, c_A);
        K = mbAddrA->sample_Y->get_value_xy(r_A + 2, c_A);
        L = mbAddrA->sample_Y->get_value_xy(r_A + 3, c_A);
    }
    matrix p(5,9,0);
    p[1][0] = I; p[2][0] = J; p[3][0] = K; p[4][0] = L;
    
    for (uint8_t i = 0; i < 4; i++)
    {
        pred.Set_r(i, p[i + 1][0]);
    }

}
void macroblock::Prediction_Intra4x4_DC                  (matrix& pred, int index)
{
    int16_t A = -1, B = -1, C = -1, D = -1;
    int16_t I = -1, J = -1, K = -1, L = -1;
    int predIndex = 0;
    int r_H = 0, c_H = 0;
    int r_V = 0, c_V = 0;
    macroblock* mbAddrA = pic->get_BLneighbour(this, 'A', index, 0x011, predIndex, r_H, c_H);
    macroblock* mbAddrB = pic->get_BLneighbour(this, 'B', index, 0x011, predIndex, r_V, c_V);
    if(this->is_avaiable(mbAddrB))
    {
        A = mbAddrB->sample_Y->get_value_xy(r_V, c_V + 0);
        B = mbAddrB->sample_Y->get_value_xy(r_V, c_V + 1);
        C = mbAddrB->sample_Y->get_value_xy(r_V, c_V + 2);
        D = mbAddrB->sample_Y->get_value_xy(r_V, c_V + 3);
    }
    if(this->is_avaiable(mbAddrA))
    {
        I = mbAddrA->sample_Y->get_value_xy(r_H + 0, c_H);
        J = mbAddrA->sample_Y->get_value_xy(r_H + 1, c_H);
        K = mbAddrA->sample_Y->get_value_xy(r_H + 2, c_H);
        L = mbAddrA->sample_Y->get_value_xy(r_H + 3, c_H);
    }
    int value = 0;
    if(A != -1 && B != -1 && C != -1 && D != -1 && I != -1 && J != -1 && K != -1 && L != -1)
    {value = ((int)(A + B + C + D + I + J + K + L + 4)) >> 3;}
    else if((A == -1 || B == -1 || C == -1 || D == -1) && (I != -1 && J != -1 && K != -1 && L != -1))
    {
        value =  ((int)(I + J + K + L + 2)) >> 2;
    }
    else if((A != -1 && B != -1 && C != -1 && D != -1) && (I == -1 || J == -1 || K == -1 || L == -1))
    {
        value = ((int)(A + B + C + D + 2)) >> 2;
    }
    else 
    value = (1 << (parser_g->pV->BitDepthY - 1));

    for(uint8_t i = 0; i < 16; i++){pred.get_value_i(i) = value;}
}
void macroblock::Prediction_Intra4x4_Diagonal_Down_Left  (matrix& pred, int index)
{
    uint8_t A = 0, B = 0, C = 0, D = 0;
    uint8_t E = 0, F = 0, G = 0, H = 0;
    int predIndex = 0;
    int r_B = 0, c_B = 0;
    int r_C = 0, c_C = 0;
    macroblock* mbAddrB = pic->get_BLneighbour(this, 'B', index, 0x011, predIndex, r_B, c_B);
    macroblock* mbAddrC = pic->get_BLneighbour(this, 'C', index, 0x011, predIndex, r_C, c_C);
    if(this->is_avaiable(mbAddrB))
    {
        A = mbAddrB->sample_Y->get_value_xy(r_B, c_B + 0);
        B = mbAddrB->sample_Y->get_value_xy(r_B, c_B + 1);
        C = mbAddrB->sample_Y->get_value_xy(r_B, c_B + 2);
        D = mbAddrB->sample_Y->get_value_xy(r_B, c_B + 3);
    }
    if(this->is_avaiable(mbAddrC))
    {
        if(index == 3 || index == 11)
        {
            E = F = G = H = D;
        }
        else 
        {
            E = mbAddrC->sample_Y->get_value_xy(r_C, c_C + 0);
            F = mbAddrC->sample_Y->get_value_xy(r_C, c_C + 1);
            G = mbAddrC->sample_Y->get_value_xy(r_C, c_C + 2);
            H = mbAddrC->sample_Y->get_value_xy(r_C, c_C + 3);

        }
    }
    else {E = F = G = H = D;}
    matrix p(1,8,0);
    p[0][0] = A;p[0][1] = B;p[0][2] = C;p[0][3] = D;p[0][4] = E;p[0][5] = F;p[0][6] = G;p[0][7] = H;
    for (uint8_t i = 0; i < 4; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            if(i == 3 && j == 3)pred[i][j] = (p[0][6] + 3 * p[0][7] + 2) >> 2;
            else pred[i][j] = (p[0][i+j] + 2 * p[0][i+j+1] + p[0][i+j+2] + 2) >> 2;
        }
    }
}
void macroblock::Prediction_Intra4x4_Diagonal_Down_Right (matrix& pred, int index)
{
    matrix p(5,9,0);
    get_4x4Neighbour(p, index);
    for(uint8_t i = 1; i < 5; i++)
    {
        for (uint8_t j = 1; j < 5; j++)
        {
            if(j - 1 > i - 1)
            pred[i-1][j-1] = (\
            p[0][(j - 1) - (i - 1) - 2 + 1] + \
            2 * p[0][(j - 1) - (i - 1) - 1 + 1] + \
            p[0][(j - 1) -(i - 1) + 1] + 2) >> 2;
            else if (j -1 < i - 1)
            pred[i-1][j-1] = \
            (p[(i - 1) - (j - 1) - 2 + 1][0]\
             + 2 * p[(i - 1) - (j - 1) - 1 + 1][0]\
              + p[(i - 1) - (j - 1) + 1][0] + 2) >> 2;
            else
            pred[i-1][j-1] = (p[0][1] + 2 * p[0][0] + p[1][0] + 2) >> 2;
        }
    }
}
void macroblock::Prediction_Intra4x4_V_Right             (matrix& pred, int index)
{
    
    matrix p(5,9,0);
    get_4x4Neighbour(p, index);
    for (uint8_t i = 1; i < 5; i++)
    {
        for (uint8_t j = 1; j < 5; j++)
        {
            int zVR = 2 * (j - 1) - (i - 1);
            if(zVR == 0 || zVR == 2 || zVR == 4 || zVR == 6)
            pred[i-1][j-1] = (p[0][(j - 1) - ((i - 1) >> 1) - 1 + 1] + p[0][(j - 1) - ((i - 1) >> 1) + 1] + 1) >> 1;
            else if(zVR == 1 || zVR ==3 || zVR == 5)
            pred[i-1][j-1] = (p[0][(j - 1) - ((i - 1) >> 1) - 2 + 1] + 2 * p[0][(j - 1) - ((i - 1) >> 1) - 1 + 1] + p[0][(j - 1) - ((i - 1) >> 1) + 1] + 2) >> 2;
            else if(zVR == -1)
            pred[i-1][j-1] = (p[1][0] + 2 * p[0][0] + p[0][1] + 2) >> 2;
            else 
            pred[i-1][j-1] = (p[(i - 1) - 1 + 1][0] + 2 * p[(i - 1) - 2 + 1][0] + p[(i - 1) - 3 + 1][0] + 2) >> 2;
        }
    }
}
void macroblock::Prediction_Intra4x4_H_Down              (matrix& pred, int index)
{

    matrix p(5,9,0);
    get_4x4Neighbour(p, index);

    for (uint8_t i = 1; i < 5; i++)
    {
        for (uint8_t j = 1; j < 5; j++)
        {
            int zHD = 2 * (i - 1) - (j - 1);
            if(zHD == 0 || zHD == 2 || zHD == 4 || zHD == 6)
            pred[i-1][j-1] = (p[(i - 1) - ((j - 1) >> 1) - 1 + 1][0] + p[(i - 1) - ((j - 1) >> 1) + 1][0] + 1) >> 1;
            else if (zHD == 1 || zHD == 3 || zHD == 5)
            pred[i-1][j-1] = (p[(i - 1) - ((j - 1) >> 1) - 2 + 1][0] + 2 * p[(i - 1) - ((j - 1) >> 1) - 1 + 1][0]\
              + p[(i - 1) - ((j - 1) >> 1) + 1][0] + 2) >> 2;
            else if(zHD == -1)
            pred[i-1][j-1] = (p[1][0] + 2 * p[0][0] + p[0][1] + 2) >> 2;
            else 
            pred[i-1][j-1] = (p[0][(j - 1) - 1 + 1] + 2 * p[0][(j - 1) - 2 + 1] + p[0][(j - 1) - 3 + 1] + 2) >> 2;
        }
    }
}
void macroblock::Prediction_Intra4x4_V_Left              (matrix& pred, int index)
{
    matrix p(5,9,0);
    get_4x4Neighbour(p, index);
    for (uint8_t i = 1; i < 5; i++)
    {
        for (uint8_t j = 1; j < 5; j++)
        {
            if((i - 1) == 0 || (i - 1) == 2)
            pred[i-1][j-1] = (\
            p[0][(j - 1) + ((i - 1) >> 1) + 1] + \
            p[0][(j - 1) + ((i - 1) >> 1) + 1 + 1] + \
            1\
            ) >> 1;
            else
            pred[i-1][j-1] = (\
            p[0][(j - 1) + ((i - 1) >> 1) + 1] + \
            2 * p[0][(j - 1) + ((i - 1) >> 1) + 1 + 1] + \
            p[0][(j - 1) + ((i - 1) >> 1) + 2 + 1] + \
            2\
            ) >> 2;
        }
    }
    
}
void macroblock::Prediction_Intra4x4_H_Up                (matrix& pred, int index)
{
    
    matrix p(5,9,0);
    get_4x4Neighbour(p, index);
    for (uint8_t i = 1; i < 5; i++)
    {
        for (uint8_t j = 1; j < 5; j++)
        {
            //pred坐标不需要变换，但是p坐标需要变换，两轴上的值分别加上1
            //这是矩阵对象只能先访问列带来的问题，
            int zHU = (j - 1) + 2 * (i - 1);
            if(zHU == 0 || zHU == 2 || zHU == 4)
            pred[i-1][j-1] = (p[(i - 1) + ((j - 1) >> 1) + 1][0] + p[(i - 1) + ((j - 1) >> 1) + 1 + 1][0] + 1) >> 1;
            else if(zHU == 1 || zHU ==3)
            pred[i-1][j-1] = (p[(i - 1) + ((j - 1) >> 1) + 1][0] + 2 * p[(i - 1) + ((j - 1) >> 1) + 1 + 1][0] + p[(i - 1) + ((j - 1) >> 1) + 2 + 1][0] + 2) >> 2;
            else if(zHU == 5)
            pred[i-1][j-1] = (p[2 + 1][0] + 3 * p[3 + 1][0] + 2) >> 2;
            else 
            pred[i-1][j-1] = p[3 + 1][0];
        }
    }
}



void macroblock::Prediction_Intra8x8()
{

}
void macroblock::Prediction_Intra16x16()
{
    switch (macroBlockInfo_I_slice[mb_type][2])
    {
    case 0:Prediction_Intra16x16_V(); break;
    case 1:Prediction_Intra16x16_H(); break;
    case 2:Prediction_Intra16x16_DC(); break;
    case 3:Prediction_Intra16x16_Plane(); break;
    default:break;
    }
}
void macroblock::Prediction_Intra16x16_V()
{
    macroblock* B = pic->get_MBneighbour(this, 'B');
    if(this->is_avaiable_forIntra16x16(B))
    for (uint8_t c = 0; c < 16; c++)
    {
        (*pred_Y).Set_c(c, (*(B->sample_Y))[15][c]);
    }
}
void macroblock::Prediction_Intra16x16_H()
{
    macroblock* A = pic->get_MBneighbour(this, 'A');
    if(this->is_avaiable_forIntra16x16(A)) 
    for (uint8_t r = 0; r < 16; r++)
    {
        (*pred_Y).Set_r(r, (*(A->sample_Y))[r][15]);
    }
}
void macroblock::Prediction_Intra16x16_DC()
{
    macroblock* A = pic->get_MBneighbour(this, 'A');
    macroblock* B = pic->get_MBneighbour(this, 'B');
    if(this->is_avaiable_forIntra16x16(A) && this->is_avaiable_forIntra16x16(B))  
        (*pred_Y) += ((A->sample_Y->Sum_c(15) + B->sample_Y->Sum_r(15) + 16) >> 5);
    else if(this->is_avaiable_forIntra16x16(A)) 
        (*pred_Y) += ((A->sample_Y->Sum_c(15) + 8) >> 4);
    else if(this->is_avaiable_forIntra16x16(B)) 
        (*pred_Y) += ((B->sample_Y->Sum_c(15) + 8) >> 4);
    else (*pred_Y) += (1 << (parser_g->pV->BitDepthY - 1));
}
void macroblock::Prediction_Intra16x16_Plane()
{
    int16_t H_samples[17] = {};
    int16_t V_samples[17] = {};
    macroblock* D = pic->get_MBneighbour(this, 'D');
    if(this->is_avaiable(D)) H_samples[0] = V_samples[0] = (*(D->sample_Y))[15][15];

    macroblock* A = pic->get_MBneighbour(this, 'A');
    if(this->is_avaiable(A)){for (uint8_t i = 0; i < 16; i++){V_samples[i + 1] = (*(A->sample_Y))[i][15];}}
    macroblock* B = pic->get_MBneighbour(this, 'B');
    if(this->is_avaiable(B)){for (uint8_t i = 0; i < 16; i++){H_samples[i + 1] = (*(B->sample_Y))[15][i];}}

    int H = 0, V = 0;
    for (uint8_t i = 9; i < 17; i++){H += (i - 8) * (H_samples[i] - H_samples[16 - i]);}
    for (uint8_t i = 9; i < 17; i++){V += (i - 8) * (V_samples[i] - V_samples[16 - i]);}
    
    int a = 16 * (H_samples[16] + V_samples[16]);
    int b = (5 * H + 32) >> 6;
    int c = (5 * V + 32) >> 6;

    for (uint8_t row = 0; row < 16; row++)
    {
        for (uint8_t col = 0; col < 16; col++)
        {
            (*pred_Y)[row][col] = Clip1Y(((a + b * (col - 7) + c * (row - 7) + 16) >> 5),parser_g->pV->BitDepthY);
        }
    }
}

MbTypeName macroblock::get_type(){return type;}


bool macroblock::is_avaiable(macroblock* mb_N)
{
    if(mb_N == NULL) return false;
    if(this->idx_inpicture < mb_N->idx_inpicture ||\
       this->id_slice != mb_N->id_slice) return false;
    else return true;
}
bool macroblock::is_avaiable_forIntra16x16(macroblock* mb_N)
{
    if(!this->is_avaiable(mb_N) || ((mb_N->is_interpred() || (mb_N->type == SI_M)) && 0/*这个是帧内预测限制条件，现在还没写*/)) return false;
    else return true;
}

uint8_t macroblock::get_PosByIndex(int index, int& r, int& c)
{
    uint8_t index_Raster = 0;
    index_Raster = block4x4Index[index];
    r = (index_Raster / 4) * 4;
    c = (index_Raster % 4) * 4;
    return index_Raster;
}
void macroblock::ConstructPicture()
{

    //每一次进来都初始化一次，4x4的值就不会叠加起来
    //初始的是重建的样点值，而不是预测值
    *sample_Y *= 0;
    *sample_U *= 0;
    *sample_V *= 0;
    *sample_Y += *pred_Y;
    *sample_Y += *(re->residual_Y);
    *sample_U += *pred_U;
    *sample_U += *(re->residual_U);
    *sample_V += *pred_V;
    *sample_V += *(re->residual_V);
}
macroblock::macroblock()
{
}
macroblock::macroblock(Slice* parent, Parser* parser)
{
    this->up_slice = parent;
    this->parser_g = parser;
    this->pic = parent->uppernal->pic;
    this->next_macroblock = NULL;
    pred_Y   = new matrix(16,16,0);
    pred_U   = new matrix(8,8,0);
    pred_V   = new matrix(8,8,0);
    sample_Y = new matrix(16,16,0);
    sample_U = new matrix(8,8,0);
    sample_V = new matrix(8,8,0);
    transform_size_8x8_flag = 0;

    sub_mb_type = NULL;
    mb_qp_delta = 0;
    Intra4x4PredMode = NULL;
    Intra8x8PredMode = NULL;
    //帧内预测限制标志，没找到这个句法元素在哪里
    constrained_intra_pred_flag = 0;
    //前置一个预测模式为帧内预测留下空间
    premode = Pred_NU;
    //宏块的分区默认设置为只有一个，也就是最大的宏块
    num_MBpart = 1;

    ref_idx_l0 = NULL;
    ref_idx_l1 = NULL;

    mv_l0      = NULL;
    mv_l1      = NULL;
    mvd_l0     = NULL;
    mvd_l1     = NULL;

    rem_intra4x4_pred_mode       = NULL;
    prev_intra4x4_pred_mode_flag = NULL;

    CodedBlockPatternLuma = 0;
    CodedBlockPatternChroma = 0;
}
macroblock::~macroblock()
{
    Sdelete_l(Intra4x4PredMode);
    Sdelete_l(Intra8x8PredMode);
    Sdelete_s(pred_Y  );
    Sdelete_s(pred_U  );
    Sdelete_s(pred_V  );
    Sdelete_s(sample_Y);
    Sdelete_s(sample_U);
    Sdelete_s(sample_V);

    Sdelete_s(re);

    Sdelete_s(sub_mb_type);
    Sdelete_l(ref_idx_l0 );
    Sdelete_l(ref_idx_l1 );

    Sdelete_l(mv_l0  );
    Sdelete_l(mv_l1  );
    Sdelete_l(mvd_l0);
    Sdelete_l(mvd_l1);

    

    Sdelete_l(rem_intra4x4_pred_mode      );
    Sdelete_l(prev_intra4x4_pred_mode_flag);
}