#ifndef MACROBLOCK_H__
#define MACROBLOCK_H__


#include <iostream>
#include "matrix.h"
#include "enums.h"
#include "array2d.h"
class residual;
class picture;
class Parser;
class Slice;

class macroblock
{
private:
    uint8_t mb_type;
    
    uint32_t address;
    Parser* parser_g;

    //帧间预测
    //块索引，子块索引， 块宽，块高，参考图片索引，运动矢量，是否预测标志
    //直接修改宏块的预测值，
    void Prediction_Inter(matrix& out, uint8_t mbPartIdx, uint8_t subMbPartIdx, uint8_t width_part, uint8_t height_part, picture* idx_ref, matrix* mv_lx, bool predFlag);
    int Prediction_Inter_LumaSampleInterpolation(int xIntL, int yIntL, int xFracL, int yFracL, picture* ref_pic);

    uint8_t residual_mode;
    //4x4预测方式，一共16个
    uint8_t* rem_intra4x4_pred_mode;
    uint8_t* prev_intra4x4_pred_mode_flag;
    void Prediction_Intra4x4(int index);
    void ParseIntra4x4PredMode();
    void ParseIntra4x4(matrix& pred, int mode, int index);
    //sample 是输出的值，必须初始化一个(5,9,0)的矩阵，
    matrix& get_4x4Neighbour(matrix& sample, int index);
    /*
    +------------------> j
    |M  A_B_C_D_E_F_G_H
    |I |              x
    |J |       
    |K |       
    |L v  y
    v
      i
    坐标轴变换：i = y + 1; j = x + 1;
    */
    void Prediction_Intra4x4_V                   (matrix& pred, int index);
    void Prediction_Intra4x4_H                   (matrix& pred, int index);
    void Prediction_Intra4x4_DC                  (matrix& pred, int index);
    void Prediction_Intra4x4_Diagonal_Down_Left  (matrix& pred, int index);
    void Prediction_Intra4x4_Diagonal_Down_Right (matrix& pred, int index);
    void Prediction_Intra4x4_V_Right             (matrix& pred, int index);
    void Prediction_Intra4x4_H_Down              (matrix& pred, int index);
    void Prediction_Intra4x4_V_Left              (matrix& pred, int index);
    void Prediction_Intra4x4_H_Up                (matrix& pred, int index);

    void Prediction_Intra8x8();

    void Prediction_Intra16x16();
    void Prediction_Intra16x16_DC();
    void Prediction_Intra16x16_Plane();
    void Prediction_Intra16x16_H();
    void Prediction_Intra16x16_V();
    
    void Weight_CoefficWeight(bool is_explicit, matrix& m0, matrix& m1, matrix& out, int , int, bool flag_0, bool falg_1);
    void Weight_defaultWeight(                  matrix& m0, matrix& m1, matrix& out,            bool flag_0, bool falg_1);
    void ConstructPicture();

    // int* residual(uint8_t startIdx, uint8_t endIdx);
    bool is_avaiable_forIntra16x16(macroblock* current);

    void Calc_residual();
    struct Info_inter* info_inter;

    uint8_t get_numpart();

    MbTypeName get_type();

public:
    Slice* up_slice;
    bool  TransformBypassModeFlag;

    uint16_t intra_chroma_pred_mode;
    uint8_t transform_size_8x8_flag;
    //
    //use to find mb in pic
    uint16_t position_x;
    uint16_t position_y;
    uint16_t idx_inpicture;
    //use to find mb in slice(though slice is free)
    uint8_t  id_slice;
    uint16_t idx_inslice;

    uint8_t constrained_intra_pred_flag;
    //syntax elements
    uint16_t mb_skip_flag;           //已经在宏块中得到
    uint16_t mb_field_decoding_flag; //from slice  not get yet !!!!!! in slice.cpp  line183
    uint8_t coded_block_pattern;   
    uint8_t CodedBlockPatternLuma;
    uint8_t CodedBlockPatternChroma;

    int* ref_idx_l0   ;
    int* ref_idx_l1   ;
    //第一个[]是子块的索引，第二个[]是子子块的索引，第三个[]是组件的索引
    matrix* mvd_l0;
    matrix* mvd_l1;
    matrix* mv_l0;
    matrix* mv_l1;

    uint8_t* Intra4x4PredMode;
    uint8_t* Intra8x8PredMode;
    //used in residual
    int16_t mb_qp_delta;
    int8_t  QPY_prev;
    int8_t  QPY;
    int8_t  QPY_;
    int8_t  QPC;
    int8_t  QPC_;
    //faind next in slice
    macroblock* next_macroblock;
    
    MbTypeName type;
    SubMbTypeName* sub_mb_type  ;

    uint8_t num_MBpart;
    MbPartPredMode premode;

    //data
    residual* re;
    matrix* pred_Y;
    matrix* pred_U;
    matrix* pred_V;
    matrix* residual_Y;
    matrix* residual_U;
    matrix* residual_V;
    matrix* sample_Y;
    matrix* sample_U;
    matrix* sample_V;
    matrix* cons_sample_Y;
    matrix* cons_sample_U;
    matrix* cons_sample_V;

    picture* pic;
    //for current , whether the target is avaiable 
    //this 必须是current，mb_N是相邻块
    bool is_avaiable(macroblock* mb_N);
    bool is_interpred(){return (type >= I_NxN && type < I_PCM) ? false : true;};
    bool is_intrapred(){return (type >= I_NxN && type < I_PCM) ? true : false;};
    //从index得到这个块的上左样点的位置（相对于本宏块），第二第三的参数分别是r和c，返回值是index的逆扫描顺序(如果用得上的话)
    uint8_t get_PosByIndex(int index, int& r, int& c);

    //the macroblock can judge the decode type by itself
    void Calc(int);
    void Decode(int);
    void Parse(int);
    //为skip宏块准备的初始化函数
    void Init0(int);
    //打印宏块的信息
    void Info();
    macroblock();
    macroblock(Slice* parent, Parser* parser);
    ~macroblock();
};






#endif