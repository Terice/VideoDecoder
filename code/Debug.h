#ifndef DEBUG_H__
#define DEBUG_H__

#include <string>
#include <stdio.h>
#include <ctime>
class Debug
{
private:
    clock_t upr_t;
    clock_t cur_t;
    std::string info_error;
    char de_cabac_state_running            ;
    char de_cabac_result_ae                ;
    char de_cabac_result_bin               ;
    char de_cabac_result_residual          ;

    char de_residual_transcoeff            ;

    char de_macroblcok                     ;

    char de_residual_result                ;
    char de_residual_result_Y              ;
    char de_residual_result_Cb             ;
    char de_residual_result_Cr             ;

    char de_prediction_result              ;
    char de_prediction_result_Y            ;
    char de_prediction_result_Cb           ;
    char de_prediction_result_Cr           ;

    char de_conspic_result                 ;
    char de_conspic_result_Y               ;
    char de_conspic_result_Cb              ;
    char de_conspic_result_Cr              ;

    char de_inter_movevector               ;
    char de_pic_terminalchar               ;

    char de_timer                          ;
    char de_nal_info                       ;

    bool control_all;
public:
    void set_control_all(bool control){control_all = control;}

    bool cabac_result_bin()        {if(control_all) return de_cabac_result_bin;     else return false;};
    bool cabac_state_running()     {if(control_all) return de_cabac_state_running;  else return false;};
    bool cabac_result_ae()         {if(control_all) return de_cabac_result_ae;      else return false;};
    bool cabac_result_residual()   {if(control_all) return de_cabac_result_residual;else return false;};
    bool residual_transcoeff()     {if(control_all) return de_residual_transcoeff;  else return false;};
    bool macroblock_all()          {if(control_all) return de_macroblcok;           else return false;};
    bool residual_result_Y()       {if(control_all) return de_residual_result_Y;    else return false;};
    bool residual_result_Cb()      {if(control_all) return de_residual_result_Cb;   else return false;};
    bool residual_result_Cr()      {if(control_all) return de_residual_result_Cr;   else return false;};
    bool prediction_result_Y()     {if(control_all) return de_prediction_result_Y;  else return false;};
    bool prediction_result_Cb()    {if(control_all) return de_prediction_result_Cb; else return false;};
    bool prediction_result_Cr()    {if(control_all) return de_prediction_result_Cr; else return false;};
    bool conspic_result_Y()        {if(control_all) return de_conspic_result_Y;     else return false;};
    bool conspic_result_Cb()       {if(control_all) return de_conspic_result_Cb;    else return false;};
    bool conspic_result_Cr()       {if(control_all) return de_conspic_result_Cr;    else return false;};
    bool inter_movevector()        {if(control_all) return de_inter_movevector ;    else return false;};
    bool pic_terminalchar()        {if(control_all) return de_pic_terminalchar ;    else return false;};
    bool timer()                   {if(control_all) return de_timer            ;    else return false;};
    bool nal_info()                {if(control_all) return de_nal_info         ;    else return false;};

    double get_RunTime();
    double de_DltTime(const char* stage);
    Debug(const char*);
    ~Debug();
};
#endif
