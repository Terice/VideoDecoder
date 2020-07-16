
#include "Debug.h"


extern "C"{
#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>
}
#define DEBUG_CONF "./Debug_conf.lua"
inline bool getControlValue(lua_State* L, int idex)
{
    bool result = false;
    if(!lua_isnumber(L, idex)) return result;
    else {
        result = (bool)(int)lua_tonumber(L, idex);
        return result;
    }
}
//已经废弃
//以类似于C的格式配置的文件读取每一个控制符
bool getControlValue(FILE* fp)
{
    int count = 0;
    char tmp;
    while((tmp = fgetc(fp)) != ';' && tmp != EOF)
    {
        if(tmp == '/') 
        do
        {
            tmp = fgetc(fp);
        } while (tmp == '\n');
        continue;
    }
    if(tmp == EOF) return false;
    else count++;
    fseek(fp, -2L, SEEK_CUR);
    tmp = fgetc(fp);
    fgetc(fp);
    return (bool)(tmp - 48);
}
Debug::Debug(const char* filename_config)
{
    lua_State* L = lua_open();
    luaL_openlibs(L);
    int state = 1;//lua运行状态
    //先load一次lua文件，然后执行一次匿名函数（也就是这个文件）
    if(luaL_loadfile(L, DEBUG_CONF) || lua_pcall(L, 0,0,0))
    {
        state = 0;
        printf("lua state machie error\n");
        info_error = "cant open lua machine";
    }
    //设置栈顶
    lua_settop(L, 0);
    //对于控制符的强制控制等处理由lua的函数来完成
    //入栈一次获得配置的函数，结果在lua的栈中
    lua_getglobal(L, "Get_conf");
    int config_length = 19;
    //调用这个函数，返回到lua栈中
    lua_pcall(L, 0,config_length,0);
    //控制符的地址依次放入一个char指针里面，
    char* parameters[] = {
        &de_cabac_state_running        ,
        &de_cabac_result_bin           ,
        &de_cabac_result_ae            ,
        &de_cabac_result_residual      ,

        &de_residual_transcoeff        ,

        &de_macroblcok                 ,

        &de_residual_result_Y          , 
        &de_residual_result_Cb         ,
        &de_residual_result_Cr         ,

        &de_prediction_result_Y        ,
        &de_prediction_result_Cb       ,
        &de_prediction_result_Cr       ,

        &de_conspic_result_Y           ,
        &de_conspic_result_Cb          ,
        &de_conspic_result_Cr          ,

        &de_inter_movevector           ,
        &de_pic_terminalchar           ,
        &de_timer                      ,
        &de_nal_info                   ,
    };
    if(!state)
    {
        for (size_t i = 0; i < config_length; i++)
        {*(parameters[i]) = 0;}
    }
    else
    {
        //用lua的栈按按顺序返回这些控制符
        for (size_t i = 0; i < config_length; i++)
        {*(parameters[i]) = getControlValue(L, i+1);}//因为栈是从1开始的，所以i+1
    }
    //关闭lua虚拟机
    lua_close(L);
    control_all = true;
    //获取运行初始时间
    upr_t = clock();
}

double Debug::get_RunTime()
{

}
double Debug::de_DltTime(const char* stage)
{
    double result = 0.0;
    if(de_timer)
    {
        cur_t = clock();
        result = (double)(cur_t - upr_t)/CLOCKS_PER_SEC;
        upr_t = cur_t;
        printf(">>Debug: RunTime: %fs,   stage:%s\n", result, stage);
    }
    return result;
}
Debug::~Debug()
{
}