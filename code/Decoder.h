#ifndef DECODER_H__
#define DECODER_H__

class picture;
class matrix;
class Slice;
#include "array2d.h"
#include <vector>
#include <stack>
#include "enums.h"
#include "functions.h"

class Decoder
{
private:
    std::vector<picture*> list_Decoded;
    //参考帧列表0 和 1 中分别应该保存解码列表的引用而不是值，这样可以直接操作到解码队列
    std::vector<picture*> list_Ref;
    //升序排列long
    std::vector<picture*> list_Ref_long;
    //升序排列short
    std::vector<picture*> list_Ref_short;
    //当前正在解码的帧
    picture* pic_current;
    //上一个已经标记好的参考帧
    picture* pic_uprefer;
    Slice* cur_slice;

    bool ctrl_MMOC_1(int);
    bool ctrl_MMOC_2(int);
    bool ctrl_MMOC_3(int ,int);
    bool ctrl_MMOC_4(int);
    bool ctrl_MMOC_5();
    bool ctrl_MMOC_6(int);
    bool flsh_ListRef();
public:
    //放在public用来让pic选择参考图像
    std::vector<picture*> list_Ref0;
    std::vector<picture*> list_Ref1;
    int MaxLongTermFrameIdx;

    matrix* matrix_4x4Trans;
    matrix* matrix_2x2Trans;
    picture* get_CurrentPic(){return pic_current;};
    void     set_CurrentPic(picture* pic){pic_current = pic;};

    picture* get_DeocdePic (int i){return list_Decoded[i];};
    picture* get_Ref0PicByI(int i){return *(list_Ref0.end() - i - 1);};
    picture* get_Ref1PicByI(int i){return *(list_Ref1.end() - i - 1);};

    void set_CurSlcie(Slice* to){cur_slice = to;}
    //解码队列
    //没有维护，遇到IDR图片就刷新，除此之外不停把新解码图片指针加入到队列尾
    bool add_DecodedPic(picture* pic_toadd){list_Decoded.push_back(pic_toadd); return true;};
    bool add_ReferenPic(picture* pic_toadd);
    //先释放pic(pic包含所有的宏块数据) 然后清除解码队列
    bool clear_DecodedPic();
    //计算图像序号相应的变量
    bool calc_PictureNum();
    //初始化参考列表，需要参数指定当前片的类型，按照当前片的类型来初始化相应的参考表
    bool init_RefPicList();

    bool ctrl_Memory();

    bool ctrl_FIFO(int);
    std::vector<int> opra_MMOC;
    bool ctrl_MMOC();
    
    //第一个是最大pic数量，第二个是当前pic的picnum
    bool opra_RefModfication(int MaxPicNum, int  CurrPicNum, int num_ref_idx_lX_active_minus1, bool);
    std::vector<int> opra_ModS;

    //打印参考列表
    void print_list();
    Decoder();
    ~Decoder();
};
#endif