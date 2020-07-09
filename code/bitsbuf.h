#ifndef BITSBUF_H_
#define BITSBUF_H_

#include<iostream>
#include<stdio.h>
using std::endl;
using std::cout;
#define MAXBUFSIZE 32
typedef unsigned char uchar;

/*

*/
class Bitsbuf
{
private:
    uchar* buf_data;
    uchar buf1_data[MAXBUFSIZE];
    uchar* buf2_data;
    uint8_t state = 0;//防止竞争状态
    bool buf_state = 0;//0-empty 1-full
    bool buf2_state = 0;//0-empty 1-full
    int buf_index = 0;//the first bit
    uchar buf_errmsg;
    FILE* datares;
    void bseti(char indextoset);
    
    bool bimax(){return buf_index == MAXBUFSIZE * 8 ? true : false;};

    void bufin(FILE* fp);//take bitstream into buf
    void bufin(uchar* ch);
    void bufin();
    bool bempt();
public:

    Bitsbuf();
    Bitsbuf(FILE* datares);
    ~Bitsbuf();
    //读入一个bit
    bool bread();
    //读入一个字节
    //会强制对齐一次
    u_char bread_ch();
    //判断是否对齐
    bool balgi(){return buf_index % 8 == 0 ? true : false;};
    //强制向后字节位对齐
    //如果没有对齐，那么索引去掉8的余数，然后加1
    bool bread_al(){if(!balgi()) {buf_index -= buf_index%8;buf_index+=8;} return true; };

    uint64_t bread_n(uchar size);
    uint64_t bnext(uint32_t nextsize);

    int64_t  bread_se();//signed golomb
    uint64_t bread_ue();//unsigned golomb
    uint64_t bread_me(uint16_t ChromaArrayType, uint32_t mb_type);//映射指数
    uint64_t bread_te(uint32_t range);//截断指数

    uint64_t bread_ce();//上下文自适应的二进制算数熵编码 CAVLC
    uint64_t bread_ae();//上下文自适应的可变长熵编码    CABAC
    
    int64_t  bread_in(uint16_t n);
    uint64_t bread_un(uint16_t n);
    uint64_t bread_fn(uint16_t n);

    

    void bfrsh();

    uchar berror();
};





#endif