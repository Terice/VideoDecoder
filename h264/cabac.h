#ifndef CABAC_H__
#define CABAC_H__


#include <iostream>

#include "array2d.h"
class cabac
{
private:
    class Debug* debug;

    uint8_t b3, b1;
    class Slice* lifeTimeSlice;
    array2d<uint8_t>* ctxIdxOfInitVariables;
    uint8_t state;// 0 - sleep,   1 - active;
    class Parser* p;
    class picture* pic;
    uint16_t codIRange;
    uint16_t codIOffset;
    uint8_t bypassFlag;
    uint16_t ctxIdx;
    uint16_t ctxIdxInc;
    uint16_t ctxIdxOffset;

    uint8_t init_variable();
    uint8_t init_engine();
    uint16_t DecodeCtxIdxUsingBinIdx(uint16_t binIdx, uint16_t maxBinIdxCtx, int ctxIdxOffset, int syntaxRequest);
    bool DecodeValueUsingCtxIdx(uint16_t ctxIdx_value, uint8_t bypassFlag_value);
    void RenormD();
    uint16_t read_intra_chroma_pred_mod();
    uint8_t read_prev_intra4x4_pred_mode_flag();
    uint8_t read_rem_intra4x4_pred_mode();

    uint8_t read_mb_skip_flag();

    uint16_t read_mb_type();
    //子宏块预测中的值
    uint8_t read_sub_mb_type();
    uint8_t read_ref_idx(int);
    int read_mvd_lx(int);

    uint16_t read_coded_block_pattern();
    uint8_t read_transform_8x8_size_flag();
    int8_t read_mb_qp_delta();
    uint8_t read_coded_block_flag(int);
    uint8_t read_significant_coeff_flag(int);
    uint8_t read_last_significant_coeff_flag(int);
    uint8_t read_coeff_sign_flag(int);
    uint32_t read_coeff_abs_level_minus1(int);

    // 二值化的方法
    // 基于查表的方法
    bool Binarization_mbtype_submbtype(uint16_t &maxBinIdxCtx, int &ctxIdxOffset, uint8_t &bypassFlag);
    // 一元二值化
    int IsIn_U_binarization(uint64_t value, uint8_t length);    //if is, return value, if not, return -1
    // 截断一元二值化
    int IsIn_TU_binarization(uint32_t value, uint8_t cMax, uint8_t length);//if is, return value, if not, return -1
    // k阶指数哥伦布二值化
    //判断是不是在UEGk二值串中，如果是，返回这个二值串的值，否则返回-1
    //五个参数分别是：当前的值，当前的长度，signedValFlag uCoeff k由句法表得到
    int IsIn_UEGk_binarization(uint32_t value, uint8_t length, uint8_t  signedValFlag, uint8_t uCoeff, uint8_t k);

    uint16_t pStateIdx;
    uint16_t valMPS   ;
    uint32_t Decode(int);
public:
    uint8_t get_state(){return state;};
    int cread_ae(int);
    bool slice_end();
    bool set_pic(class picture* p);
    bool set_slice(class Slice* sl);
    cabac(Parser* parser);
    ~cabac();
};

#endif