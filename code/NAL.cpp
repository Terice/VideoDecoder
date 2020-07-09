
#include "NAL.h"
#include "Parser.h"
#include "array2d.h"
#include "cabac.h"
#include "slice.h"
#include "picture.h"
using std::endl;
using std::cout;
#include <cmath>

#include "Decoder.h"
#include "Debug.h"

bool NAL::decode()
{   
    if(parser == NULL) 
    {
        cout << "no res " << endl;
        return false;
    }
    forbidden_zero_bit = parser->read_un(1);
    nal_ref_idc = parser->read_un(2);
    nal_unit_type = parser->read_un(5);
    type = (NALtype)nal_unit_type;


    switch (type)
    {
        case IDR:decode_PIC();break;
        case Non_IDR:decode_PIC();break;
        case SPS:decode_SPS();break;
        case PPS:decode_PPS();break;
        default:break;
    }
    return true;
}
void NAL::decode_SPS()
{
    SPS_data* data = parser->pS->sps;
    
    data->profile_idc                             = parser->read_un(8);//
    data->constraint_set0_flag                    = parser->read_un(1);//
    data->constraint_set1_flag                    = parser->read_un(1);//
    data->constraint_set2_flag                    = parser->read_un(1);//
    data->constraint_set3_flag                    = parser->read_un(1);//
    data->constraint_set4_flag                    = parser->read_un(1);//
    data->constraint_set5_flag                    = parser->read_un(1);//
    data->reserved_zero_2bits                     = parser->read_un(2);//
    data->level_idc                               = parser->read_un(8);//
    data->seq_parameter_set_id                    = parser->read_ue();//
    if(data->profile_idc==100||data->profile_idc==110||data->profile_idc==122\
     ||data->profile_idc==244||data->profile_idc==44 ||data->profile_idc==83\
     ||data->profile_idc==86 ||data->profile_idc==118||data->profile_idc==128\
     ||data->profile_idc==138||data->profile_idc==139||data->profile_idc==134)
    {
        data->chroma_format_idc                   = parser->read_ue();//
        data->separate_colour_plane_flag          = parser->read_ue();//
        data->bit_depth_luma_minus8               = parser->read_ue();//
        data->bit_depth_chroma_minus8             = parser->read_un(1);//
        data->qpprime_y_zero_transform_bypass_flag= parser->read_un(1);//
        data->seq_scaling_matrix_present          = parser->read_un(1);//
        data->seq_scaling_list_present_flag       = 0;//
    }
    data->log2_max_frame_num_minus4               = parser->read_ue();//
    data->pic_order_cnt_type                      = parser->read_ue();//
    if(data->pic_order_cnt_type == 0) 
    {
        data->log2_max_pic_order_cnt_lsb_minus4   = parser->read_ue();//
    }
    else if(data->pic_order_cnt_type == 1)
    {
        data->delta_pic_order_always_zero_flag      = parser->read_un(1);//
        data->offset_for_non_ref_pic                = parser->read_se();//
        data->ffset_for_top_to_bottom_field         = parser->read_se();//
        data->num_ref_frames_in_pic_order_cnt_cycle = parser->read_ue();//
        Sdelete_l(data->offset_for_ref_frame);

        data->offset_for_ref_frame = new uint32_t[data->num_ref_frames_in_pic_order_cnt_cycle];
        for (size_t i = 0; i < data->num_ref_frames_in_pic_order_cnt_cycle; i++)
        {
            data->offset_for_ref_frame[i]         = parser->read_ue();//
        }
        
    }
    data->max_num_ref_frames                      = parser->read_ue();//
    data->gaps_in_frame_num_value_allowe_flag     = parser->read_un(1);//
    data->pic_width_in_mbs_minus1                 = parser->read_ue();//
    data->pic_height_in_map_units_minus1          = parser->read_ue();//
    data->frame_mbs_only_flag                     = parser->read_un(1);//
    if(!data->frame_mbs_only_flag) 
    {
        data->mb_adaptive_frame_field_flag         = parser->read_un(1);//
    }
    data->direct_8x8_inference_flag               = parser->read_un(1);//
    data->frame_cropping_flag                     = parser->read_un(1);//
    if(data->frame_cropping_flag)
    {
        data->frame_crop_left_offset              = parser->read_ue();//
        data->frame_crop_right_offset             = parser->read_ue();//
        data->frame_crop_top_offset               = parser->read_ue();//
        data->frame_crop_bottom_offset            = parser->read_ue();//
    }
    data->vui_parameters_present_flag             = parser->read_un(1);//

    this->data = data;


    parser->pV->ChromaArrayType     = data->separate_colour_plane_flag == 0 ? data->chroma_format_idc : 0;
    parser->pV->BitDepthY           = data->bit_depth_luma_minus8 + 8;
    parser->pV->QpBdOffsetY         = data->bit_depth_luma_minus8 * 6;
    parser->pV->BitDepthC           = data->bit_depth_chroma_minus8 + 8;
    parser->pV->QpBdOffsetC         = data->bit_depth_chroma_minus8 * 6;
    parser->pV->MaxFrameNum         = (uint32_t)pow(2, data->log2_max_frame_num_minus4 + 4);
    parser->pV->PicWidthInMbs       = data->pic_width_in_mbs_minus1 + 1;
    parser->pV->PicHeightInMapUnits = data->pic_height_in_map_units_minus1 + 1;
    parser->pV->PicHeightInMbs       = data->pic_height_in_map_units_minus1 + 1;
    parser->pV->FrameHeightInMbs    = ( 2 - data->frame_mbs_only_flag ) * parser->pV->PicHeightInMapUnits;
    parser->pV->PicSizeInMapUnits   = parser->pV->PicWidthInMbs *parser->pV->PicHeightInMapUnits;
    
    parser->pV->SubWidthC           = (data->chroma_format_idc == 1 || data->chroma_format_idc == 2) ? 2 : 1;
    parser->pV->SubHeightC          = (data->chroma_format_idc == 1) ? 2 : 1;
}
void NAL::decode_PPS()
{
    PPS_data* data = (parser->pS->pps);
    data->pic_parameter_set_id                                   = parser->read_ue();//
    data->seq_parameter_set_id                                   = parser->read_ue();//
    data->entropy_coding_mode_flag                               = parser->read_un(1);//
    data->bottom_field_pic_order_in_frame_present_flag           = parser->read_un(1);
    data->num_slice_groups_minus1                                = parser->read_ue();//
    if(data->num_slice_groups_minus1 > 0)
    {
        data->slice_group_map_type                               = parser->read_ue();//
        if(data->slice_group_map_type == 0)
        {
            Sdelete_s(data->run_length_minus1);

            data->run_length_minus1 = new uint32_t[data->num_slice_groups_minus1];
            for (size_t iGroup = 0; iGroup < data->num_slice_groups_minus1; iGroup++)
            {
                data->run_length_minus1[iGroup]                  = parser->read_ue();//
            }
        }
        else if(data->slice_group_map_type ==2)
        {

            Sdelete_s(data->top_left     );
            Sdelete_s(data->bottom_right );

            data->top_left     = new uint32_t[data->num_slice_groups_minus1];
            data->bottom_right = new uint32_t[data->num_slice_groups_minus1];
            for(size_t iGroup=0;iGroup < data->num_slice_groups_minus1;iGroup++)
            {
                data->top_left[iGroup]                           = parser->read_ue();//
                data->bottom_right[iGroup]                       = parser->read_ue();//
            }
        }
        else if(data->slice_group_map_type==3||data->slice_group_map_type==4||data->slice_group_map_type==5)
        {
            data->slice_group_change_direction_flag              = parser->read_un(1);//
            data->slice_group_change_rate_minus1                 = parser->read_ue();//
        }
        else if(data->slice_group_map_type == 6)
        {
            data->pic_size_in_map_units_minus1                   = parser->read_ue();//
            Sdelete_s(data->slice_group_id);
            data->slice_group_id = new uint32_t[data->pic_size_in_map_units_minus1];
            for (size_t i = 0; i < data->pic_size_in_map_units_minus1 ; i++)
            {
                data->slice_group_id[i]                          = parser->read_un(4);//
            }
        }
    }
    data->num_ref_idx_l0_default_active_minus1                   = parser->read_ue();//
    data->num_ref_idx_l1_default_active_minus1                   = parser->read_ue();//
    data->weighted_pred_flag                                     = parser->read_un(1);//
    data->weighted_bipred_idc                                    = parser->read_un(2);//
    data->pic_init_qp_minus26                                    = parser->read_se();//
    data->pic_init_qs_minus26                                    = parser->read_se();//
    data->chroma_qp_index_offset                                 = parser->read_se();//
    data->deblocking_fliter_control_present_flag                 = parser->read_un(1);//
    data->constrained_intra_pred_flag                            = parser->read_un(1);//
    data->redundant_pic_cnt_present_flag                         = parser->read_un(1);//
    if(0)
    {
        data->transform_8x8_mode_flag                                = parser->read_un(1);//
    }

    
    this->data = data;
    
}
uchar** NAL::decode_PIC()
{
    //如果是IDR，清空所有队列的所有pic指针，释放所有的pic内存
    if(type == IDR) decoder->clear_DecodedPic();

    picture* picture_cur = new picture(parser->pS->sps->pic_width_in_mbs_minus1+1, parser->pS->sps->pic_height_in_map_units_minus1+1, type == IDR ? 1 : 0);
    this->pic = picture_cur;

    
    parser->cabac_core->set_pic(picture_cur);

    Slice* sl1 = new Slice(this->parser, this->decoder ,this);
    sl1->PraseSliceHeader();
    

    //slice头只有一个，所以slice和pic其实有很多重合的属性，但是比较关键的是属性应该是位于pic中而不是slice中的
    
    //每次解完头，把frame_num这个句法赋值给pic，然后用decoder去解picture numbers
    pic->FrameNum = sl1->ps->frame_num;
    
    //解码picture numbers
    decoder->calc_PictureNum(parser->pV->MaxFrameNum, sl1->ps->field_pic_flag);
    
    decoder->ctrl_Memory();
    //然后再把当前的pic加入到已经完成解码的队列中(虽然当前pic还没有完全解码)
    decoder->add_DecodedPic(pic);
    //初始化参考列表，把图片加入到相应的参考列表中去
    Slicetype sl1_type = sl1->get_type();

    //解I帧不需要参考，只需要标记
    //只是排序的话，按照长期短期索引来排序就行了把，


    //初始化参考表(根据上一次的标记)
    if(sl1_type == P || sl1_type == SP || sl1_type == B)
    decoder->init_RefPicList(sl1_type);



    sl1->PraseSliceDataer();
    //片解码完毕，宏块已经入pic
    

    
    //解码完毕需要对当前的pic进行标记，
    //在下一次解码的时候会重排序和修改，而不是这里
    //这里只是标记而已
    if(nal_ref_idc != 0)
    {
        decoder->add_ReferenPic(pic);
        //I：所有的片已经解码
        //不考虑场的问题，到这里就已经是完成解码了
        //II
        if(pic->is_IDR())
        {
            //I第一步所有参考pic标记为不用于参考
            //II
            if(sl1->ps->long_term_reference_flag == 0)
            {
                decoder->MaxLongTermFrameIdx = -1;
                pic->state_Ref += 1;
            }
            else
            {
                decoder->MaxLongTermFrameIdx = 0;
                pic->state_Ref += 10;
                pic->LongTermFrameIdx = 0;
            }
        }
        else 
        {
            //标记的时候
            if(sl1->ps->adaptive_ref_pic_marking_mode_flag == 0)
            {/*滑窗标记*/}
            else
            {/*自适应标记*/}
        }
        //III
        //进入到这里的pic都是需要标记和参考的长期帧
        //如果不是由内存控制标记标记的长期帧，那么标记为短期帧
        //内存控制标记的长期帧有着IDR的作用，
        //其他一律标记为短期帧
        if(!pic->is_IDR() && !(pic->state_Ref / 10 != 1 && pic->memory_management_control_operation == 6))
        {
            pic->state_Ref += 1;
        }
    }
    if(sl1_type != B) 
    {
        if(parser->debug->pic_terminalchar())
        {
            //像素字符化
            if((type == IDR || type == Non_IDR) && sl1_type != B)
            {
                pic->chs_MbToOutmatrix();
                cout << (*pic) << endl;
            }
        }
        if(parser->debug->nal_info())
        {
            printf(">>nal  : type: %s, ref_idc: %2d, \n", type==IDR?"IDR":"NONE_IDR", nal_ref_idc);
            decoder->print_list();
            printf(">>slice: type: %2d, index: %2d\n", sl1->get_type(), sl1->get_index());
            printf("----nal divide line----\n\n");
        }
    }
    else
    {
    }
    
    Sdelete_s(sl1);
    return NULL;
}



const void* NAL::getdata()
{
    return this->data;
}

NAL::NAL()
{

}
NAL::NAL(Parser* parser, Decoder* decoder)
{
    this->parser = parser;
    this->decoder = decoder;
    this->parser->cur_nal = this;
}
NAL::~NAL()
{
}