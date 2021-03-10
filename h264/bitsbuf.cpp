#include "bitsbuf.h"
#include <cmath>

static const uint16_t me_chart12[48][2] = {
    { 47, 0  }, { 31, 16 }, { 15, 1  }, {  0, 2  }, 
    { 23, 4  }, { 27, 8  }, { 29, 32 }, { 30, 3  }, 
    {  7, 5  }, { 11, 10 }, { 13, 12 }, { 14, 15 }, 
    { 39, 47 }, { 43, 7  }, { 45, 11 }, { 46, 13 }, 
    { 16, 14 }, {  3, 6  }, {  5, 9  }, { 10, 31 }, 
    { 12, 35 }, { 19, 37 }, { 21, 42 }, { 26, 44 }, 
    { 28, 33 }, { 35, 34 }, { 37, 36 }, { 42, 40 }, 
    { 44, 39 }, {  1, 43 }, {  2, 45 }, {  4, 46 }, 
    {  8, 17 }, { 17, 18 }, { 18, 20 }, { 20, 24 }, 
    { 24, 19 }, {  6, 21 }, {  9, 26 }, { 22, 28 }, 
    { 25, 23 }, { 32, 27 }, { 33, 29 }, { 34, 30 }, 
    { 36, 22 }, { 40, 25 }, { 38, 38 }, { 41, 41 } 
};
static const uint16_t me_chart03[16][2] = {
    { 15, 0 }, {  0, 1 }, {  7, 2 }, { 11, 4 }, { 13, 8 },
    { 14, 3 }, {  3, 5 }, {  5, 10}, { 10, 12}, { 12, 15}, 
    {  1, 7 }, {  2, 11}, {  4, 13}, {  8, 14}, {  6, 6 },
    {  9, 9 }
};

//读入一个char
short Bitsbuf::bread_ch()
{
    short result = 0;
    //强制对齐一次
    bread_al();
    //如果结束位不是最大位，并且当前位大于结束位(也就是说出现了EOF)
    //那么返回-1
    if(eofsize < 8 * MAXBUFSIZE && buf_index >= eofsize)
        return -1;
    //如果是在边界,那么刷新一次缓存区
    if(buf_index == MAXBUFSIZE * 8) 
        bfrsh();
    result = buf_data[buf_index/8];
    //索引后移动8位
    buf_index += 8;

    return result;

}
bool Bitsbuf::bread()
{
    if(bimax()) bfrsh();
    size_t charIndex = buf_index / 8;
    int bitsIndex = buf_index % 8;
    bool result = 1;

    if(!buf_state)
    {
        result &= buf_data[charIndex] >> (8 - bitsIndex - 1); 
        if(++buf_index == MAXBUFSIZE * 8)
        {bfrsh();}
    }
    else std::cout<<"buf empty"<<std::endl;
    // cout<<"bit1"<<endl;
    // cout<<(int)buf_index<<(int)result<<endl;
    return result;
}
uint64_t Bitsbuf::bread_n(uchar size)
{
    size_t charSize = size / 8;
    size_t bitsSize = size % 8;
    uint64_t result = 0, tmp = 0;

    if(charSize > 0)
    {
        charSize -= 1;
        bitsSize += 8;
    }
    while(bitsSize > 0 && buf_index % 8)
    {
        tmp += bread();
        result += tmp << (8 * (charSize) + bitsSize - 1);
        tmp = 0;
        bitsSize--;
    }
    while(charSize > 0)
    {
        tmp += buf_data[buf_index / 8];
        result += tmp << (8 * (charSize - 1) + bitsSize);
        tmp = 0;
        charSize--;

        buf_index += 8;
        if(buf_index == MAXBUFSIZE * 8) 
        {
            bfrsh();
        }
    }
    while(bitsSize > 0)
    {
        tmp += bread();
        result += tmp << (bitsSize - 1);
        tmp = 0;
        bitsSize--;
    }
    return result;
    
}
void Bitsbuf:: bfrsh()
{ 
    
    if(!buf2_state)
    {
        // cout<<">>buf  :refresh the file"<<endl;
        bufin((FILE*)datares);
        // printf("%x, %x, %x, %x\n", buf_data[0], buf_data[1], buf_data[2], buf_data[3]);
    }
    else
    {
        bufin(buf2_data);
        buf2_state = 0;
        delete[] buf2_data;
        // cout<<">>buf  :read from buf2"<<endl;
        
    }
    buf_index = 0;
}

void Bitsbuf::bufin(FILE* fp)
{
    if(fp != NULL) 
    {
        int ch = 0;
        for(size_t count = 0;count < MAXBUFSIZE;count++)
        {
            ch = fgetc(fp);
            if(ch == EOF) 
                eofsize = count * 8;

            //去掉防止竞争字节的状态机
            //0表示正常，1表示出现第一个0x00， 2表示出现第二个0x00并且等待下一个03   3表示出现03   4表示出现 0x01 或者 0x02 或者 0x03
            switch (state)
            {
            case 0:if(ch == 0) state = 1; break;
            case 1:if(ch == 0) state = 2; else state = 0; break;
            case 2:if(ch == 3) state = 3; else if(ch == 0) state = 2; else state = 0; break;//如果出现03那么下一个状态，如果当前仍然是0，那么维持状态，否则解除状态
            case 3:if(ch == 1 || ch == 2 || ch == 3 || ch == 0) state = 4; else state = 0; break;
            default:state = 0; break;
            }
            //如果出现防止竞争序列，索引后退一个char，当前字符读入0x03位置上，然后count自增
            if(state == 4)
            {
                count--;
                //如果防止竞争的最后一位是0，那么删除03之后的序列为0x00 00 00
                //仍然是一个危险的序列，需要等待下一个03，所以状态为2
                if(ch == 0) state = 2;
                else state = 0;
            }
            buf_data[count] = (u_char)ch;
        }
        //如果最后一个字节是危险的字节，
        //那么尝试读取下一个字符来判断这个字节是否应该去掉
        if(state == 3)
        {
            ch = fgetc(fp);
            if(ch == 1 || ch == 2 || ch == 3 || ch == 0)
            {
                buf_data[MAXBUFSIZE - 1] = (u_char)ch;
                state = 0;
            }
            else {fseek(fp, -1L, SEEK_CUR);}
        }
    }
    else printf("no input file");
}
void Bitsbuf::bufin(uchar* ch)
{
    if(ch != NULL)
    {
        for (size_t i = 0; i < MAXBUFSIZE; i++)
        {
            buf_data[i] = ch[i];
        }
    }
}
bool Bitsbuf::bempt()
{
    for (size_t i = 0; i < MAXBUFSIZE; i++)
    {
        buf_data[i] = 0;
    }
    return true;
}
void Bitsbuf::bseti(char deltaindex)
{
    buf_index += deltaindex;
}
uint64_t Bitsbuf::bnext(uint32_t nextsize)
{
    if(nextsize > MAXBUFSIZE * 16) cout<<"!!!-->out of buf"<<endl;

    if(!buf2_state)
    {
        buf2_data = new uchar[MAXBUFSIZE];
        fread(buf2_data, sizeof(char), MAXBUFSIZE, (FILE*)datares);
        buf2_state = 1;

    }

    uint32_t curIndex = buf_index;
    uint64_t result = bread_n(nextsize);
    
    buf_index = curIndex;
    buf_data = buf1_data;

    return result;
}
uint64_t Bitsbuf::bread_ue()
{
    uint16_t M = 0;
    uint32_t M_cur = 0;
    uint32_t num_cur = 0;
    uint64_t result = 0;

    while ((num_cur = bread()) == 0) M++;
    result += (num_cur << M);
    for (M_cur = M; M_cur > 0 && M > 0; M_cur--)
    {
        num_cur = bread();
        result += num_cur << (M_cur - 1);
    }
    
    result -= 1;
    return result;
    
}
int64_t Bitsbuf::bread_se()
{
    uint16_t M = 0;
    uint32_t M_cur = 0;
    float num_cur = 0;
    int64_t result = 0;

    num_cur = bread_ue();
    result = (int64_t)pow((-1), (num_cur + 1)) * (int64_t)(ceil(num_cur / 2));
    return result;
}
uint64_t Bitsbuf::bread_te(uint32_t range)
{
    if(range > 1) return bread_ue();
    else 
    {
        if(bread() == 1) return 0;
        else return 1;
    }
}
uint64_t Bitsbuf::bread_me(uint16_t ChromaArrayType, uint32_t mb_type)
{
    uint64_t result;
    result = bread_ue();
    if(ChromaArrayType == 0 || ChromaArrayType == 3) return me_chart03[result][mb_type];
    else return me_chart12[result][mb_type];
}
//bool Bitsbuf ::balgi(){return buf_index % 8 == 0 ? true : false;}


Bitsbuf::Bitsbuf()
{
    cout<<"init a buf"<<endl;
}
Bitsbuf::Bitsbuf(FILE* datares)
{
    eofsize = MAXBUFSIZE * 8;
    state = 0;
    cout<<"data res is: "<<(FILE*)datares<<endl;
    this->datares = datares;
    buf_data = buf1_data;
    bfrsh();
}
Bitsbuf::~Bitsbuf()
{
}

