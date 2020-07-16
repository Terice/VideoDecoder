
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
    forbidden_zero_bit = parser->read_un(1);
    nal_ref_idc = parser->read_un(2);
    nal_unit_type = parser->read_un(5);
    type = (NALtype)nal_unit_type;

    switch (type)
    {
        case Non_IDR:decode_PIC();break;
        case IDR:decode_PIC();break;
        case PPS:decode_PPS();break;
        case SPS:decode_SPS();break;
        default:printf("error or nowrite nal");break;
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
    parser->pV->PicHeightInMbs      = data->pic_height_in_map_units_minus1 + 1;
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
    //
    //如果是IDR，清空所有队列的所有pic指针，释放所有的pic内存
    if(type == IDR) decoder->clear_DecodedPic();

    picture* picture_cur = new picture(parser->pS->sps->pic_width_in_mbs_minus1+1, parser->pS->sps->pic_height_in_map_units_minus1+1, type == IDR ? 1 : 0);
    this->pic = picture_cur;
    //设置解码器的当前图像，用于MMOC中的CurPicNum
    decoder->set_CurrentPic(picture_cur);

    //设置CABAC的解码对象
    parser->cabac_core->set_pic(picture_cur);

    //初始化一个slice之后就解码头部，
    //（slcie头没有依赖的数据，可以先解）
    Slice* sl1 = new Slice(this->parser, this->decoder ,this);
    decoder->set_CurSlcie(sl1);
    sl1->PraseSliceHeader();
    Slicetype sl1_type = sl1->get_type();
    
    

    //每次解完头，把frame_num这个句法赋值给pic，然后用decoder去解picture numbers
    pic->FrameNum = sl1->ps->frame_num;
    //解码picture numbers
    decoder->calc_PictureNum();

    //初始化参考列表，把pic_current指针加入到相应的参考列表中去
    //解I帧不需要参考，只需要标记
    //初始化参考表(根据上一次的标记)
    if(sl1_type == P || sl1_type == SP || sl1_type == B)
    decoder->init_RefPicList();

    //修改
    //操作符在 decoder 中已经指定和如何移除，所以这里不用做是否启用这个函数的判定
    decoder->opra_RefModfication(sl1->ps->MaxPicNum, sl1->ps->CurrPicNum, sl1->ps->num_ref_idx_l0_active_minus1, 0);
    decoder->opra_RefModfication(sl1->ps->MaxPicNum, sl1->ps->CurrPicNum, sl1->ps->num_ref_idx_l1_active_minus1, 1);

    //至此参考列表建立完成
    //去掉解码队列中无用的帧
    decoder->ctrl_Memory();

    //slice数据块解码依赖于参考列表所以放在参考列表建立完之后
    sl1->PraseSliceDataer();
    //片解码完毕，宏块在pic上

    decoder->add_DecodedPic(pic);

    //图像解码完毕，pic进入解码队列
    //decoder的pic_current指针指向这个解码完毕的pic
    
    //解码完毕需要对当前的pic进行标记，
    //在下一次解码的时候会初始化，重排序和修改，而不是这里
    if(nal_ref_idc != 0)
    {
        //第一步：所有的片已经解码
        //不考虑场的问题，到这里就已经是完成解码了

        //第二步：参考队列中的pic的标记
        if(pic->is_IDR())
        {
            //I所有参考pic标记为不用于参考
            //II
            if(sl1->ps->long_term_reference_flag == 0)
            {
                decoder->MaxLongTermFrameIdx = -1;
                pic->state_Ref = Ref_short;
            }
            else
            {
                decoder->MaxLongTermFrameIdx = 0;
                pic->state_Ref = Ref_long;
                pic->LongTermFrameIdx = 0;
            }
        }
        else
        {
            //标记
            if(sl1->ps->adaptive_ref_pic_marking_mode_flag == 0)
            //滑窗标记操作：帧队列满那么去掉最小PicNum的pic
            {
                decoder->ctrl_FIFO(parser->pS->sps->max_num_ref_frames);
                /*滑窗标记*/
            }
            else//自适应标记在slice头就会完成，
            {
                decoder->ctrl_MMOC();
                /*自适应标记*/
            }
        }

        //第三步，当前pic的标记
        //自适应内存标记6号就是标记当前pic
        //如果不是     由内存控制标记6标记的长期帧     ，
        //那么标记为短期帧
        if(!pic->is_IDR() && !(pic->is_UsedForLong() && pic->memory_management_control_operation == 6))
        {
            pic->state_Ref = Ref_short;
        }

        //标记完毕，加入到参考队列中并分别加入参考表
        decoder->add_ReferenPic(pic);
    }

    if(sl1_type != B) 
    {
        //像素字符化并输出pic
        if(parser->debug->pic_terminalchar())
        {
            if((type == IDR || type == Non_IDR) && sl1_type != B)
            {
                pic->chs_MbToOutmatrix();
                std::cout << (*pic) << std::endl;
            }
        }
        //输出:
        //nal的基本信息，
        //参考队列
        //slice的基本信息
        if(parser->debug->nal_info())
        {
            printf(">>nal  : type: %s, ref_idc: %2d, \n", type==IDR?"IDR":"NONE_IDR", nal_ref_idc);
            decoder->print_list();
            printf(">>slice: type: %2d, index: %2d\n", sl1->get_type(), sl1->get_index());
            printf("----nal divide line----\n\n");
        }
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