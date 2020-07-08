#ifndef SPS_H__
#define SPS_H__
#include <iostream>
typedef struct SPSDATA
{
    uint32_t  profile_idc                          = 0;//
    uint32_t  constraint_set0_flag                 = 0;//
    uint32_t  constraint_set1_flag                 = 0;//
    uint32_t  constraint_set2_flag                 = 0;//
    uint32_t  constraint_set3_flag                 = 0;//
    uint32_t  constraint_set4_flag                 = 0;//
    uint32_t  constraint_set5_flag                 = 0;//
    uint32_t  reserved_zero_2bits                  = 0;//
    uint32_t  level_idc                            = 0;//
    uint32_t  seq_parameter_set_id                 = 0;//
    uint32_t  chroma_format_idc                    = 1;//
    uint32_t  separate_colour_plane_flag           = 0;//
    uint32_t  bit_depth_luma_minus8                = 0;//
    uint32_t  bit_depth_chroma_minus8              = 0;//
    uint32_t  qpprime_y_zero_transform_bypass_flag = 0;//
    uint32_t  seq_scaling_matrix_present           = 0;//
    uint32_t  seq_scaling_list_present_flag        = 0;//
    uint32_t  log2_max_frame_num_minus4            = 0;//
    uint32_t  pic_order_cnt_type                   = 0;//
    uint32_t  log2_max_pic_order_cnt_lsb_minus4    = 0;//
    uint32_t  delta_pic_order_always_zero_flag     = 0;//
    uint32_t  offset_for_non_ref_pic               = 0;//
    uint32_t  ffset_for_top_to_bottom_field        = 0;//
    uint32_t  num_ref_frames_in_pic_order_cnt_cycle= 0;//
    uint32_t* offset_for_ref_frame                 = NULL;//
    uint32_t  max_num_ref_frames                   = 0;//
    uint32_t  gaps_in_frame_num_value_allowe_flag  = 0;//
    uint32_t  pic_width_in_mbs_minus1              = 0;//
    uint32_t  pic_height_in_map_units_minus1       = 0;//
    uint32_t  frame_mbs_only_flag                  = 0;//
    uint32_t  mb_adaptive_frame_field_flag         = 0;//
    uint32_t  direct_8x8_inference_flag            = 0;//
    uint32_t  frame_cropping_flag                  = 0;//
    uint32_t  frame_crop_left_offset               = 0;//
    uint32_t  frame_crop_right_offset              = 0;//
    uint32_t  frame_crop_top_offset                = 0;//
    uint32_t  frame_crop_bottom_offset             = 0;//
    uint32_t  vui_parameters_present_flag          = 0;//
}SPS_data;

#endif