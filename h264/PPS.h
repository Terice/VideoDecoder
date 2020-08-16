#ifndef PPS_H__
#define PPS_H__
#include <iostream>
typedef struct PPSDATA
{
    uint32_t  pic_parameter_set_id                                                           = 0;//
    uint32_t  seq_parameter_set_id                                                           = 0;//
    uint32_t  entropy_coding_mode_flag                                                       = 0;//
    uint16_t  bottom_field_pic_order_in_frame_present_flag                                   = 0;//
    uint32_t  num_slice_groups_minus1                                                        = 0;//
    uint32_t  slice_group_map_type                                                           = 0;//
    uint32_t* run_length_minus1                                                              = NULL;//
    uint32_t* top_left                                                                       = NULL;//
    uint32_t* bottom_right                                                                   = NULL;//
    uint32_t  slice_group_change_direction_flag                                              = 0;//
    uint32_t  slice_group_change_rate_minus1                                                 = 0;//
    uint32_t  pic_size_in_map_units_minus1                                                   = 0;//
    uint32_t* slice_group_id                                                                 = NULL;//
    uint32_t  num_ref_idx_l0_default_active_minus1                                           = 0;//
    uint32_t  num_ref_idx_l1_default_active_minus1                                           = 0;//
    uint32_t  weighted_pred_flag                                                             = 0;//
    uint32_t  weighted_bipred_idc                                                            = 0;//
    int32_t   pic_init_qp_minus26                                                            = 0;//
    int32_t   pic_init_qs_minus26                                                            = 0;//
    int32_t   chroma_qp_index_offset                                                         = 0;//
    uint32_t  deblocking_fliter_control_present_flag                                         = 0;//
    uint32_t  constrained_intra_pred_flag                                                    = 0;//
    uint32_t  redundant_pic_cnt_present_flag                                                 = 0;//
 
    uint8_t   transform_8x8_mode_flag                                                        = 0;//
}PPS_data;
#endif


