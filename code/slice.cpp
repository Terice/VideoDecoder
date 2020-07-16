#include "Debug.h"

#include "slice.h"
#include <cmath>
#include "Parser.h"
#include "NAL.h"
#include "macroblock.h"
#include "cabac.h"
#include "picture.h"
#include <iostream>

#include "Decoder.h"
#include "functions.h"
void Slice::PraseSliceHeader()
{
    int v;
    ps->first_mb_in_slice                    = parser->read_ue();
    ps->slice_type                           = parser->read_ue();
    if(ps->slice_type >= 5) type = (Slicetype)(ps->slice_type - 5);
    ps->pic_parameter_set_id                 = parser->read_ue();

    if(ps->sps->separate_colour_plane_flag == 1)
    ps->colour_plane_id                      = parser->read_un(2);

    v= ps->sps->log2_max_frame_num_minus4 + 4;
    ps->frame_num                            = parser->read_un(v);
    
    if(!ps->sps->frame_mbs_only_flag){
        ps->field_pic_flag                   = parser->read_un(1);
        if(ps->field_pic_flag){
            ps->bottom_field_flag            = parser->read_un(1);
        }
    }
    //POC
    if(uppernal->type == IDR){
        ps->idr_pic_id                       = parser->read_ue();
    }
    if(ps->sps->pic_order_cnt_type == 0){
        v = ps->sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
        ps->pic_order_cnt_lsb                = parser->read_un(v);
        if(ps->pps->bottom_field_pic_order_in_frame_present_flag && !ps->field_pic_flag)
            ps->delta_pic_order_cnt_bottom   = parser->read_se();
    }
    if(ps->sps->pic_order_cnt_type == 1 && !ps->sps->delta_pic_order_always_zero_flag){
        ps->delta_pic_order_cnt[0]           = parser->read_se();
        if(ps->pps->bottom_field_pic_order_in_frame_present_flag && !ps->field_pic_flag)
            ps->delta_pic_order_cnt[1]       = parser->read_se();
    }

    if(ps->pps->redundant_pic_cnt_present_flag){
        ps->redundant_pic_cnt                = parser->read_ue();
    }
    //B片的时间空间直接预测模式
    if(type == B){
        ps->direct_spatial_mv_pred_flag      = parser->read_un(1);;
    }
    //重载的参考帧索引数量
    if(type == P||type == SP||type== B){
        ps->num_ref_idx_active_override_flag = parser->read_un(1);
        if(ps->num_ref_idx_active_override_flag)
        {
            ps->num_ref_idx_l0_active_minus1 = parser->read_ue();
            if(type == B)
                ps->num_ref_idx_l1_active_minus1 = parser->read_ue();
        }
        else //不重载的时候使用默认的值
        {
            ps->num_ref_idx_l0_active_minus1 = parser->pS->pps->num_ref_idx_l0_default_active_minus1;
            ps->num_ref_idx_l1_active_minus1 = parser->pS->pps->num_ref_idx_l1_default_active_minus1;
        }
    }
    //参考帧重排序
    //P和SP的重排序
    if(ps->slice_type % 5 != 2 && ps->slice_type % 5 != 4 ){ 
        ps->ref_pic_list_modification_flag_l0= parser->read_un(1);
        if(ps->ref_pic_list_modification_flag_l0) 
        do {
            std::vector<int>& mods = decoder->opra_ModS;
            ps->modification_of_pic_nums_idc = parser->read_ue();
            mods.push_back(ps->modification_of_pic_nums_idc);
            if(ps->modification_of_pic_nums_idc == 0 || ps->modification_of_pic_nums_idc == 1 )
            {
                ps->abs_diff_pic_num_minus1  = parser->read_ue();
                mods.push_back(ps->abs_diff_pic_num_minus1);
            }
            else if(ps->modification_of_pic_nums_idc == 2 ) 
            {
                ps->long_term_pic_num        = parser->read_ue();
                mods.push_back(ps->long_term_pic_num);
            }
        } while(ps->modification_of_pic_nums_idc != 3 );
    }
    //B的重排序
    if(ps->slice_type % 5 == 1){ 
        ps->ref_pic_list_modification_flag_l1= parser->read_un(1);
        if( ps->ref_pic_list_modification_flag_l1 ) 
        do { 
           ps->modification_of_pic_nums_idc  = parser->read_ue();
            if(ps->modification_of_pic_nums_idc == 0 || ps->modification_of_pic_nums_idc == 1 ) 
                ps->abs_diff_pic_num_minus1  = parser->read_ue();
            else if(ps->modification_of_pic_nums_idc == 2) 
                ps->long_term_pic_num        = parser->read_ue();
        } while(ps->modification_of_pic_nums_idc != 3);
    }
    //加权预测
    if((ps->pps->weighted_pred_flag && (type == P || type == SP))||\
       (ps->pps->weighted_bipred_idc == 1 && type == B))
    {
        pw = new PredWeight();
        pw->luma_log2_weight_denom                    = parser->read_ue();
        if(parser->pV->ChromaArrayType != 0) 
            pw->chroma_log2_weight_denom              = parser->read_ue();
        pw->luma_weight_l0 = new int16_t[ps->num_ref_idx_l0_active_minus1 + 1]();
        pw->luma_offset_l0 = new int16_t[ps->num_ref_idx_l0_active_minus1 + 1]();
        pw->chroma_weight_l0 = new int16_t*[ps->num_ref_idx_l0_active_minus1 + 1]();
        pw->chroma_offset_l0 = new int16_t*[ps->num_ref_idx_l0_active_minus1 + 1]();
        for (uint8_t i = 0; i <= ps->num_ref_idx_l0_active_minus1; i++)
        {
            pw->luma_weight_l0_flag                   = parser->read_un(1);
            if(pw->luma_weight_l0_flag)
            {
                pw->luma_weight_l0[i]                 = parser->read_se();
                pw->luma_offset_l0[i]                 = parser->read_se();
            }
            else
            {
                pw->luma_weight_l0[i]                 = (int16_t)powl(2, pw->luma_log2_weight_denom);
                pw->luma_offset_l0[i]                 = 0;
            }
            if(parser->pV->ChromaArrayType != 0) 
            pw->chroma_weight_l0_flag                 = parser->read_un(1);
            if(pw->chroma_weight_l0_flag)
            {
                pw->chroma_weight_l0[i] = new int16_t[2];
                pw->chroma_offset_l0[i] = new int16_t[2];
                for (uint8_t j = 0; j < 2; j++)
                {
                    pw->chroma_weight_l0[i][j]        = parser->read_se();
                    pw->chroma_offset_l0[i][j]        = parser->read_se();
                }
                
            }
        }
        if(ps->slice_type % 5 == 1)
        {
            pw->luma_weight_l1 = new int16_t[ps->num_ref_idx_l1_active_minus1 + 1];
            pw->luma_offset_l1 = new int16_t[ps->num_ref_idx_l1_active_minus1 + 1];
            pw->chroma_weight_l1 = new int16_t*[ps->num_ref_idx_l1_active_minus1 + 1];
            pw->chroma_offset_l1 = new int16_t*[ps->num_ref_idx_l1_active_minus1 + 1];
            for (uint8_t i = 0; i <= ps->num_ref_idx_l1_active_minus1; i++)
            {
                pw->luma_weight_l1_flag               = parser->read_un(1);
                if(pw->luma_weight_l1_flag)
                {
                    pw->luma_weight_l1[i]             = parser->read_se();
                    pw->luma_offset_l1[i]             = parser->read_se();
                }
                if(parser->pV->ChromaArrayType != 0) 
                pw->chroma_weight_l1_flag             = parser->read_un(1);
                if(pw->chroma_weight_l1_flag)
                {
                    //因为色度组件有两个，所以需要2个数组
                    pw->chroma_weight_l1[i] = new int16_t[2];
                    pw->chroma_offset_l1[i] = new int16_t[2];
                    for (uint8_t j = 0; j < 2; j++)
                    {
                        pw->chroma_weight_l1[i][j]    = parser->read_se();
                        pw->chroma_offset_l1[i][j]    = parser->read_se();
                    }
                }
            }
        }
        
    }
    //参考标记
    if(uppernal->nal_ref_idc != 0)//标记
    {
        if(uppernal->type == IDR){ 
            ps->no_output_of_prior_pics_flag  = parser->read_un(1);
            ps->long_term_reference_flag      = parser->read_un(1);
        }
        else{ 
            ps->adaptive_ref_pic_marking_mode_flag = parser->read_un(1);
            //如果是自适应控制标记
            if(ps->adaptive_ref_pic_marking_mode_flag)
            {
                //自适应控制标记是一系列操作，而不是某一个操作
                int i = 0;
                do {
                    std::vector<int>& MMOC = decoder->opra_MMOC;
                    ps->memory_management_control_operation                 = parser->read_ue();
                    MMOC.push_back(ps->memory_management_control_operation);
                    //
                    if(ps->memory_management_control_operation == 1 || \
                       ps->memory_management_control_operation == 3)        
                        //和这个slice的pic相差的PicNum
                    {
                        ps->difference_of_pic_nums_minus1                   = parser->read_ue();
                        MMOC.push_back(ps->difference_of_pic_nums_minus1);
                    }
                    if(ps->memory_management_control_operation == 2) 
                        //需要移除的长期参考帧索引
                    {
                        ps->long_term_pic_num                               = parser->read_ue();  
                        MMOC.push_back(ps->long_term_pic_num);
                    }
                    if(ps->memory_management_control_operation == 3 || \
                       ps->memory_management_control_operation == 6) 
                        //需要注册的长期参考帧索引
                    {
                        ps->long_term_frame_idx                             = parser->read_ue();
                        MMOC.push_back(ps->long_term_frame_idx);
                    }
                    if(ps->memory_management_control_operation == 4)
                        //最大的长期参考索引
                    {
                        ps->max_long_term_frame_idx_plus1                   = parser->read_ue();
                        MMOC.push_back(ps->max_long_term_frame_idx_plus1);
                    }
                }while(ps->memory_management_control_operation != 0);
            }
            //否则就是滑窗标记
        }
    }
    //CABAC
    if(ps->pps->entropy_coding_mode_flag && type != I && type != SI){
        ps->cabac_init_idc                   = parser->read_ue();
    }
    ps->slice_qp_delta                       = parser->read_se();
    if(type == SP || type == SI){
        //switch解码
        if(type == SP){
        ps->sp_for_switch_flag               = parser->read_un(1);
        }
        ps->slice_qs_delta                   = parser->read_se();
    }
    //去方块滤波
    if(ps->pps->deblocking_fliter_control_present_flag)
    {
        ps->disable_deblocking_fliter_idc    = parser->read_ue();
        if(ps->disable_deblocking_fliter_idc!=1)
        {
            ps->slice_alpha_c0_offset_dic2   = parser->read_se();  
            ps->slice_beta_offset_div2       = parser->read_se();
        }
    }
    //片组循环
    
    v = ceil(log2( parser->pV->PicSizeInMapUnits / (ps->pps->slice_group_change_rate_minus1 + 1 + 1 )));
    if(ps->pps->num_slice_groups_minus1 > 0 &&\
       ps->pps->slice_group_map_type >= 3 &&\
       ps->pps->slice_group_map_type <= 5){
        ps->slice_group_change_cycle          = parser->read_un(v);
    }

    ps->MaxPicOrderCntLsb = (uint32_t)pow(2, ps->sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
    ps->SliceQPY = 26 + parser->pS->pps->pic_init_qp_minus26 + ps->slice_qp_delta;
    ps->MbaffFrameFlag = (ps->sps->mb_adaptive_frame_field_flag && !ps->field_pic_flag );
    if(ps->field_pic_flag == 0) ps->MaxPicNum = parser->pV->MaxFrameNum;
    else ps->MaxPicNum = 2 * parser->pV->MaxFrameNum; 
    if(ps->field_pic_flag == 0) ps->CurrPicNum = ps->frame_num; 
    else ps->CurrPicNum = 2 * ps->frame_num - 1; 
};
uint32_t NextMbAddress(uint32_t add){return 0;}
uint32_t more_rbsp_data(){return 1;}
void Slice::PraseSliceDataer()
{

    //初始化变量
    int x_cur = 0;
    int y_cur = 0;
    uint32_t CurrMbAddr = ps->first_mb_in_slice * (1 + ps->MbaffFrameFlag);
    uint32_t moreDataFlag = 1;
    uint32_t prevMbSkipped = 0;
    uint16_t index_MbInSlcie = 0;
    uint16_t index_MbInPicture = 0;
    //片数据对齐
    if(ps->pps->entropy_coding_mode_flag) while(!parser->algi()){parser->read_bi();}

    do{

        //跳过编码只是不解析而已，但是同样需要数据块、解码
        macroblock* mb = new macroblock(this, parser);
        if(cur_macroblcok == NULL) cur_macroblcok = mb;
        //add MB into pic
        this->uppernal->pic->add_MB(x_cur, y_cur, mb);
        //set the MB ID in slice
        mb->id_slice = parser->slice_idx;
        mb->idx_inslice = index_MbInSlcie;
        index_MbInSlcie++;
        mb->position_x = x_cur;
        mb->position_y = y_cur;

        mb->idx_inpicture = index_MbInPicture;//index_MbInPicture这个变量不应该设置在slice里面，现在还没有改过来
        //clac current next macroblock position
        index_MbInPicture++;
        y_cur++;
        if(y_cur >= parser->pV->PicWidthInMbs) {y_cur = 0; x_cur++;}

        //
        //add macroblock into slice tree and set the init MBQPY
        //slice里面的宏块树的建立，量化参数的初始化，
        if(this->head_macroblock == NULL) 
        {
            head_macroblock = mb;
            mb->QPY_prev = this->ps->SliceQPY;
        }
        else
        {
            if(cur_macroblcok->next_macroblock == NULL)
            {
                cur_macroblcok->next_macroblock = mb;
                mb->QPY_prev = cur_macroblcok->QPY;
            }
        }
        //当前宏块的设置
        cur_macroblcok = mb;
        if(type != I && type != SI)
        {
            
            if(!ps->pps->entropy_coding_mode_flag)
            {
                ps->mb_skip_run = parser->read_ue();
                prevMbSkipped = (ps->mb_skip_run > 0);
                for(size_t i = 0; i < ps->mb_skip_run; i++){
                    CurrMbAddr = NextMbAddress(CurrMbAddr);
                }
                if(ps->mb_skip_run > 0)
                    moreDataFlag = more_rbsp_data( );
            }
            else
            {
                ps->mb_skip_flag = parser->read_ae(11);
                moreDataFlag = !ps->mb_skip_flag;
            }
        }
        if(moreDataFlag)
        {
            //意思是最后的宏块对中必须有一个宏块要被解码，要么都被跳过或者被帧解码，如果解码一个的话就要用场解码
            if(ps->MbaffFrameFlag && (CurrMbAddr % 2 == 0 || (CurrMbAddr % 2 == 1 && prevMbSkipped)))
                ps->mb_field_decoding_flag = parser->read_ae(12);//1a
            else ps->mb_field_decoding_flag = ps->field_pic_flag;
            //解析宏块数据
            // parser->debug->de_DltTime("mb start");
            mb->Parse(0);
            // parser->debug->de_DltTime("mb parser");
            //所有的宏块都需要解码、重建图像
            mb->Calc(0);
            // parser->debug->de_DltTime("mb dataer");
        }
        else
        {
            //Skip宏块用下面的初始化函数保持宏块的结构
            mb->Init0(0);
            mb->Calc(0);
        }
        //打印所有宏块的信息
        mb->Info();
        if(!ps->pps->entropy_coding_mode_flag)       //if not CABAC 编码 , use more_rbsp_data() to confirm end
            moreDataFlag = more_rbsp_data();
        else
        {
            if(type != I && type != SI)
                prevMbSkipped = ps->mb_skip_flag;
            if(ps->MbaffFrameFlag && CurrMbAddr % 2 == 0)
                moreDataFlag = 1;                     //
            else
            {
                ps->end_of_slice_flag = parser->read_ae(276);            //片结束标记
                moreDataFlag = !ps->end_of_slice_flag;  //如果片结束，那么moreDataFlag为0，此循环之后停止运行
            }
        }
    }while(moreDataFlag);
    parser->slice_idx += 1;
    parser->cabac_core->slice_end();
};

void Slice::Calc_POC()
{
    uint8_t pic_order_cnt_type = parser->pS->sps->pic_order_cnt_type;
    uint8_t pic_order_cnt_lsb = ps->pic_order_cnt_lsb;
    picture* cur = decoder->get_CurrentPic();
    if(pic_order_cnt_type == 0)
    {
        int prevPicOrderCntMsb = 0,  prevPicOrderCntLsb = 0;
        if(cur->is_IDR())
        {
            prevPicOrderCntMsb = prevPicOrderCntLsb = 0;
        }
        else 
        {
            
        }
        //calc PicOrderCntMsb
        if((pic_order_cnt_lsb < prevPicOrderCntLsb) && (prevPicOrderCntLsb - pic_order_cnt_lsb >= (ps->MaxPicOrderCntLsb / 2)))
        {cur->PicOrderCntMsb = prevPicOrderCntMsb + ps->MaxPicOrderCntLsb;}
        else if((pic_order_cnt_lsb > prevPicOrderCntLsb) && (pic_order_cnt_lsb - prevPicOrderCntLsb > (ps->MaxPicOrderCntLsb / 2)))
        {cur->PicOrderCntMsb = prevPicOrderCntMsb - ps->MaxPicOrderCntLsb;}
        else cur->PicOrderCntMsb = prevPicOrderCntMsb;
        //calc top cnt, bottom cnt
        cur->TopFieldOrderCnt = cur->PicOrderCntMsb + pic_order_cnt_lsb;
        if(!ps->field_pic_flag) cur->BottomFieldOrderCnt = cur->TopFieldOrderCnt + ps->delta_pic_order_cnt_bottom;
        else cur->BottomFieldOrderCnt = cur->PicOrderCntMsb + ps->pic_order_cnt_lsb;
    }
}
//这里只是做了frame的返回，还有其他的情况没有写：自适应 和 场
macroblock* Slice::get_curMB(){return cur_macroblcok;}
macroblock* Slice::get_mbUsingIdInSlice(int32_t idInSlice)
{
    macroblock* result = this->head_macroblock;
    if(idInSlice < 0) return NULL;
    for (size_t i = 0; i < idInSlice; i++)
    {
        result = result->next_macroblock;
    }
    return result;
}
Slice::Slice(Parser* parser, Decoder* decoder,  NAL* nal)
{
    index = parser->slice_idx;
    next = NULL;
    uppernal = nal;
    head_macroblock = NULL;
    pw = NULL;
    ps = new SliceParametersets;
    ps->pps = (parser->pS->pps);
    ps->sps = (parser->pS->sps);
    this->parser = parser;
    this->decoder = decoder;
    parser->cabac_core->set_slice(this);
}
Slice::~Slice()
{
    if(pw)
    {
        Sdelete_l(pw->luma_weight_l0);
        Sdelete_l(pw->luma_offset_l0);
        if(pw->chroma_weight_l0)
        {
            for (uint8_t i = 0; i < ps->num_ref_idx_l0_active_minus1 + 1; i++)
            Sdelete_l(pw->chroma_weight_l0[i]);

            Sdelete_l(pw->chroma_weight_l0);
        }
        if(pw->chroma_offset_l0)
        {
            for (uint8_t i = 0; i < ps->num_ref_idx_l0_active_minus1 + 1; i++)
            Sdelete_l(pw->chroma_offset_l0[i]);

            Sdelete_l(pw->chroma_offset_l0);
        }
        if(pw->chroma_weight_l1)
        {            
            for (uint8_t i = 0; i < ps->num_ref_idx_l1_active_minus1 + 1; i++)
            Sdelete_l(pw->chroma_weight_l1[i]);

            Sdelete_l(pw->chroma_weight_l1);
        }
        if(pw->chroma_offset_l1)
        {
            for (uint8_t i = 0; i < ps->num_ref_idx_l1_active_minus1 + 1; i++)
            Sdelete_l(pw->chroma_offset_l1[i]);
            
            Sdelete_l(pw->chroma_offset_l1);
        }
        Sdelete_s(pw);
    }
    Sdelete_s(ps);
}