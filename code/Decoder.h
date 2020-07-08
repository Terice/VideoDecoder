#ifndef DECODER_H__
#define DECODER_H__

class picture;
class matrix;

#include "array2d.h"
#include <vector>
#include "enums.h"
#include "functions.h"

class Decoder
{
private:
    std::vector<picture*> list_Decoded;
    std::vector<picture*> list_Ref0;
    std::vector<picture*> list_Ref1;
    std::vector<picture*> list_Ref;
    picture* pic_current;
    picture* pic_uprefer;
public:

    int MaxLongTermFrameIdx;

    matrix* matrix_4x4Trans;
    matrix* matrix_2x2Trans;
    picture* get_CurrentPic(){return pic_current;};

    picture* get_DeocdePic (int i){return list_Decoded[i];};
    picture* get_Ref0PicByI(int i){return *(list_Ref0.end() - i - 1);};
    picture* get_Ref1PicByI(int i){return *(list_Ref1.end() - i - 1);};
    //解码队列
    //没有维护，遇到IDR图片就刷新，除此之外不停把新解码图片指针加入到队列尾
    bool add_DecodedPic(picture* pic_toadd){pic_current = pic_toadd; list_Decoded.push_back(pic_toadd); return true;};
    bool add_ReferenPic(picture* pic_toadd){pic_current = pic_toadd; list_Ref.push_back(pic_toadd); return true;};
    //先释放pic(pic包含所有的宏块数据) 然后清除解码队列
    bool clear_DecodedPic();
    //计算图像序号相应的变量
    bool calc_PictureNum(int maxFrameNum, int field_pic_flag);
    //初始化参考列表，需要参数指定当前片的类型，按照当前片的类型来初始化相应的参考表
    bool init_RefPicList(Slicetype type);

    bool ctrl_Memory();

    //打印参考列表
    void print_list();
    Decoder();
    ~Decoder();
};

#endif