#ifndef SLICE_H__
#define SLICE_H__


#include "SPS.h"
#include "PPS.h"
#include "enums.h"

typedef struct SlicePS
{
    
    SPS_data* sps;
    PPS_data* pps;
    uint32_t first_mb_in_slice                    ;
    uint32_t slice_type                           ;
    uint32_t pic_parameter_set_id                 ;
    uint16_t colour_plane_id                      ;
    uint32_t frame_num                            ;
    uint16_t field_pic_flag                    = 0;
    uint16_t bottom_field_flag                    ;
    uint32_t idr_pic_id                           ;
    uint16_t pic_order_cnt_lsb                    ;
    int32_t  delta_pic_order_cnt_bottom           ;
    int32_t  delta_pic_order_cnt[2]               ;
    uint16_t redundant_pic_cnt                    ;
    uint16_t direct_spatial_mv_pred_flag          ;
    uint16_t num_ref_idx_active_override_flag;    ;
    uint32_t num_ref_idx_l0_active_minus1      = 0;
    uint32_t num_ref_idx_l1_active_minus1      = 0;
    uint32_t cabac_init_idc                       ;
    int32_t  slice_qp_delta                       ;
    uint16_t sp_for_switch_flag                   ;
    uint32_t slice_qs_delta                       ;
    uint32_t disable_deblocking_fliter_idc     = 0;
    int32_t  slice_alpha_c0_offset_dic2           ;
    int32_t  slice_beta_offset_div2               ;
    uint32_t slice_group_change_cycle             ;

    uint8_t ref_pic_list_modification_flag_l0     ;
    uint8_t ref_pic_list_modification_flag_l1     ;
    uint32_t modification_of_pic_nums_idc         ;
    uint32_t abs_diff_pic_num_minus1              ;

    uint8_t  no_output_of_prior_pics_flag         ;//
    uint8_t  long_term_reference_flag             ;//
    uint8_t  adaptive_ref_pic_marking_mode_flag   ;//
    uint32_t memory_management_control_operation  ;//
    uint32_t difference_of_pic_nums_minus1        ;//
    uint32_t long_term_pic_num                    ;//
    uint32_t long_term_frame_idx                  ;//
    uint32_t max_long_term_frame_idx_plus1        ;//


    uint32_t MbaffFrameFlag                    = 0;//
    uint32_t MaxPicOrderCntLsb                 = 0;
    uint32_t MaxPicNum                         = 0;//
    uint32_t CurrPicNum                        = 0;//
    uint8_t SliceQPY                           = 0;


    //slice data
    uint32_t mb_skip_run                       = 0;//
    uint32_t mb_skip_flag                      = 0;//
    uint32_t mb_field_decoding_flag            = 0;//
    uint32_t end_of_slice_flag                 = 0;//
    
}SliceParametersets;
typedef struct PREDWEIGHT
{
    uint8_t luma_log2_weight_denom             = 0;
    uint8_t chroma_log2_weight_denom           = 0;
    uint8_t luma_weight_l0_flag                = 0;
    int16_t *luma_weight_l0                     = NULL;
    int16_t *luma_offset_l0                    = NULL;
    uint8_t chroma_weight_l0_flag              = 0;
    int16_t **chroma_weight_l0                 = NULL;
    int16_t **chroma_offset_l0                 = NULL;
    uint8_t luma_weight_l1_flag                = 0;
    int16_t *luma_weight_l1                    = NULL;
    int16_t *luma_offset_l1                    = NULL;
    uint8_t chroma_weight_l1_flag              = 0;
    int16_t **chroma_weight_l1                 = NULL;
    int16_t **chroma_offset_l1                 = NULL;

    int logWD_C                                = 0;
    int W_0c                                   = 0;
    int W_1C                                   = 0;
    int O_0C                                   = 0;
    int O_1c                                   = 0;
}PredWeight;

class Slice
{
private:
    Slicetype type;
    // cabac cabacCore;
    int index;
    class Slice* next;
    class Parser* parser;
    class macroblock* cur_macroblcok;
    class macroblock* head_macroblock;
    void Calc_POC();
public:
    class Decoder* decoder;
    class NAL* uppernal;
    SliceParametersets* ps;//SPS and PPS
    PredWeight* pw;
    Slicetype get_type(){return type;};
    int get_index(){return index;};
    class macroblock* get_curMB();
    void setpv(struct PARAMETERSV* a);
    class macroblock* get_mbUsingIdInSlice(int32_t);
    ~Slice();
    Slice(Parser* parser, Decoder* deocder, NAL* nal);
    //
    void PraseSliceHeader();
    void PraseSliceDataer();
};







#endif  