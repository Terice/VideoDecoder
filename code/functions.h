#ifndef FUNCTIONS_H__
#define FUNCTIONS_H__

#include "staticcharts.h"

//指针类型有两个括号
//safe delete single a
#define Sdelete_s(a) {if(a != NULL) delete a;a = NULL;}
//safe delete array a
#define Sdelete_l(a) {if(a != NULL) delete[] a;a = NULL;}

inline int Min(int a, int b){return a <= b ? a : b;}
inline int Max(int a, int b){return a >= b ? a : b;}
inline int Median(int x, int y, int z){return x + y + z - Min(x, Min(y, z)) - Max(x, Max(y , z));}
inline uint Abs(int v){return v >= 0 ? v : (uint)((-1)*v);}
inline int8_t Sig(int v){return v >= 0 ? 1 : -1;};
inline int MinPositive(int x, int y){return (x > 0 && y > 0) ? Min(x, y) : Max(x, y);}
inline int SixTapFliter(int a, int b, int c, int d, int e, int f){return a - 5 * b + 20 * c + 20 * d - 5 * e + f;}
inline int InverseRasterScan(int a, int b, int c, int d, int e){ return e == 0?((a%(d/b))*b):((a/(d/b))*c);}
template <typename T> inline void swap(T& a, T& b){T tmp = a; a = b; b = tmp;};

class macroblock;
bool Get_PredFlag(macroblock* m, uint8_t mbPartIdx, uint8_t flag);
//name 是块名字，a是方向
MbPartPredMode Get_MbPartPredMode(macroblock* ma, MbTypeName name, uint8_t a);
uint8_t Get_SubMbPartPredMode(SubMbTypeName name);

//只用于帧间预测计算
uint8_t Get_NumMbPart(MbTypeName mb_type);
uint8_t Get_MbPartWidth(MbTypeName mb_type);
uint8_t Get_MbPartHeight(MbTypeName mb_type);


inline uint8_t Get_SubNumMbPart(SubMbTypeName name){return name >= B_Direct_8x8 ? subMbInfo[name - 10 + 5][1] : subMbInfo[name][1];};
uint8_t Get_SubMbPartHeight(SubMbTypeName);
uint8_t Get_SubMbPartWidth(SubMbTypeName);

//Note: range: [lowerLimit, upperLimit] not:()
template <typename T>
T Clip3(T lower, T upper, T value)
{
    if(value >= upper) return upper;
    else if(value <= lower) return lower;
    else return value;
}
inline int Clip1Y(int x, int BitDepthY){return Clip3(0, (1 << BitDepthY) - 1, x);};
inline int Clip1C(int x, int BitDepthC){return Clip3(0, (1 << BitDepthC) - 1, x);};


#endif