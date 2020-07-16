#include "Decoder.h"


#include "matrix.h"
#include "picture.h"
#include "slice.h"
#include <vector>
#include <algorithm>
void Decoder::print_list()
{
    
    printf(">>deder:    (direction:----->)  [Frame_num]\n");//(dec_num)
    printf("         deocded list :");
    for (uint8_t i = 0; i < list_Decoded.size(); i++)
    {
        printf("%d, ",list_Decoded[i]->FrameNum);//, list_Decoded[i]->DecNum);
    }
    printf("\n");
    printf("         reflist   list :");
    for (uint8_t i = 0; i < list_Ref.size(); i++)
    {
        printf("%d,",list_Ref[i]->PicNum);
    }
    printf("\n");
    printf("         reflist_0 list :");
    for (uint8_t i = 0; i < list_Ref0.size(); i++)
    {
        printf("%d, ",list_Ref0[i]->PicNum);//, list_Ref0[i]->DecNum);
    }
    printf("\n");
    printf("         reflist_1 list :");
    for (uint8_t i = 0; i < list_Ref1.size(); i++)
    {
        printf("%d(%d), ",list_Ref1[i]->PicNum, list_Ref1[i]->DecNum);
    }
    printf("\n");
}
bool Decoder::clear_DecodedPic()
{
    //清空所有的队列
    for(uint8_t i = 0; i < list_Decoded.size(); i++)
    {
        Sdelete_s(list_Decoded[i]);
    }
    list_Decoded.clear(); 
    list_Ref.clear(); 
    list_Ref1.clear();
    list_Ref0.clear();
    
    return true;
};


bool Decoder::ctrl_Memory()
{
    std::vector<picture*>::iterator it= list_Decoded.begin();
    while(it != list_Decoded.end())
    {
        if(!(*it)->is_UsedForRef())
        {
            delete (*it);
            it = list_Decoded.erase(it);
        }
        else it++;
    }
    
    return true;
}

inline void swap(picture*&a, picture*& b)
{
    picture* tmp = a;
    a = b;
    b = tmp;
}
bool Decoder::add_ReferenPic(picture* pic_toadd)
{
    if(pic_toadd == NULL) return false;

    //先按照解码顺序进入总的参考队列
    list_Ref.push_back(pic_toadd);

    //然后按照参考类型分别进入相应的参考队列
    if(pic_toadd->is_UsedForLong()) list_Ref_long.push_back(pic_toadd);
    else list_Ref_short.push_back(pic_toadd);

    return true;
}

bool Decoder::flsh_ListRef()
{
    auto flsh_func = [](std::vector<picture*>&  list_toflsh)->bool{
        std::vector<picture*>::iterator it;
        it = list_toflsh.begin();
        while (it != list_toflsh.end())
        {
            if((*it) && !(*it)->is_UsedForRef())
            {
                list_toflsh.erase(it);
            }
            it++;
        }
    };
    flsh_func(list_Ref);
    flsh_func(list_Ref_short);
    flsh_func(list_Ref_long);
    
    return true;
}
bool Decoder::init_RefPicList()
{
    flsh_ListRef();
    //建表包含 初始化，排序两个工作,
    Slicetype type = cur_slice->get_type();
    if(type == P || type == SP)
    {
        std::vector<picture*>::iterator it_ref0, it;

        //清空参考队列
        list_Ref0.clear();
        //插入long
        for(it = list_Ref_short.begin(); it != list_Ref_short.end(); it++)
        {
            list_Ref0.push_back(*it);
        }
        it_ref0 = list_Ref0.end()-1;
        //从开头排序到结束的地方
        std::sort(list_Ref0.begin(), list_Ref0.end(), \
        [](picture* l, picture* r)->bool{return l->FrameNumWrap > r->FrameNumWrap;}\
        );
        //插入long
        for(it = list_Ref_long.begin(); it != list_Ref_long.end(); it++)
        {
            list_Ref0.push_back(*it);
        }
        //从短期结束的地方开始排序到结束的地方
        std::sort(it_ref0+1, list_Ref0.end(), \
        [](picture* l, picture* r)->bool{return l->LongTermFrameIdx < r->LongTermFrameIdx;}\
        );

        return true;
    }
    else //if(type == B)
    {
        
        return false;
    }
}
int PicNumF(picture* pic)
{
    if(pic && pic->is_UsedForShort()){return pic->PicNum;}
    else                             {return -1;}
}
//重排序
bool Decoder::opra_RefModfication(int MaxPicNum, int  CurrPicNum, int num_ref_idx_lX_active_minus1, bool X)
{

    int i = 0, opra = 0;
    int picNumLXPred = CurrPicNum;
    int picNumLXNoWrap = 0;
    int picNumLX = 0;
    std::vector<picture*>& RefPicListX = X?list_Ref0:list_Ref1;
    int refIdxLX = 0;

    if(opra_ModS.size() > 0)
    {
        //在末尾加上一位监视位
        RefPicListX.push_back(NULL);
        //短期重排
        while ((opra = opra_ModS[i++]) != 3)
        {
            if(opra == 0 || opra == 1)
            {
                int sig = opra == 0?-1:1;
                int diff_pic_num = (opra_ModS[i++] + 1)*sig;
                if(picNumLXPred + diff_pic_num < 0 || picNumLXPred + diff_pic_num > MaxPicNum) picNumLXNoWrap = picNumLXPred + diff_pic_num + -1*sig*MaxPicNum;
                else picNumLXNoWrap = picNumLXPred + diff_pic_num;
                picNumLXPred = picNumLXNoWrap;

                if(picNumLXNoWrap > CurrPicNum) picNumLX = picNumLXNoWrap - MaxPicNum;
                else picNumLX = picNumLXNoWrap;

                picture* tomod = NULL;
                int nIdx = 0, cIdx = 0;
                //如果最后一位不是NULL，那么说明位置不够了，需要补上一位
                if(RefPicListX[RefPicListX.size()-1] != NULL)
                {RefPicListX.push_back(NULL);}
                //整个列表向后移动一位（从refIdxLX开始的地方）
                for(cIdx = RefPicListX.size()-1; cIdx > refIdxLX; cIdx--)
                {//元素后移一位
                    RefPicListX[cIdx] = RefPicListX[cIdx - 1];
                }
                //查找满足short-term reference picture with PicNum equal to picNumLX 的pic
                std::vector<picture*>::iterator it;
                it = std::find_if(list_Ref.begin(), list_Ref.end(), \
                [picNumLX](picture* i)->bool{return i && i->is_UsedForShort() && i->PicNum == picNumLX;}\
                );
                tomod = it==RefPicListX.end()?NULL:(*it);
                //0索引赋值给 PicNum == PicNumLX 的短期参考图片，然后索引到1
                RefPicListX[refIdxLX++] = tomod;//tomod填入第一位
                nIdx = refIdxLX;
                //如果RefPicListX[cIdx]的PicNum不等于
                for(cIdx = refIdxLX; cIdx < RefPicListX.size(); cIdx++)
                    if(PicNumF(RefPicListX[cIdx]) != picNumLX)
                        RefPicListX[nIdx++] = RefPicListX[cIdx];
            }
            else //if(opra == 2)
            {//长期重排
            }
        }
        //去掉多余的元素
        while(RefPicListX.size() > num_ref_idx_lX_active_minus1 + 1 || *(RefPicListX.end()) == NULL)
        {
            RefPicListX.pop_back();
        }
    }
    //删除操作符
    opra_ModS.clear();
    return true;
}

bool Decoder::calc_PictureNum()
{
    int frame_num = cur_slice->ps->frame_num;
    int MaxFrameNum = cur_slice->ps->MaxPicNum;
    int field_pic_flag = cur_slice->ps->field_pic_flag;

    //给每个pic都计算其 相应的变量
    std::vector<picture*>::iterator it;
    for(it = list_Ref.begin(); it != list_Ref.end(); it++)
    {
        if((*it)->is_UsedForShort())
        {
            if((*it)->FrameNum > frame_num)
                (*it)->FrameNumWrap = (*it)->FrameNum - MaxFrameNum;
            else (*it)->FrameNumWrap = (*it)->FrameNum;
        }
        if(!field_pic_flag)
        {
            if((*it)->is_UsedForShort())
            {(*it)->PicNum = (*it)->FrameNumWrap;}
            if((*it)->is_UsedForLong())
            {(*it)->LongTermPicNum = (*it)->LongTermFrameIdx;}
        }
    }
    return true;
}
bool Decoder::ctrl_MMOC()
{
    /*
    1 - 一个 短期    标记为 不用于参考
    2 - 一个 长期    标记为 不用于参考
    3 - 一个 短期    标记为 长期
    4 - 大于 最大长期索引的 长期参考图像 标记为 不用于参考
    5 - 全部 参考图像 标记为 不用于参考  最大长期索引 设置为 无
    6 - 分配一个 长期参考索引 给 当前图像
    */
    opra_MMOC;
    int opration = 0;
    int i = 0;
    while (opration = opra_MMOC[i++] != 0)
    {
        switch (opration)
        {
            case 1:ctrl_MMOC_1(opra_MMOC[i++]);break;
            case 2:ctrl_MMOC_2(opra_MMOC[i++]);break;
            case 3:ctrl_MMOC_3(opra_MMOC[i++], opra_MMOC[i++]);break;
            case 4:ctrl_MMOC_4(opra_MMOC[i++]);break;
            case 5:ctrl_MMOC_5();break;
            case 6:ctrl_MMOC_6(opra_MMOC[i++]);break;
            default:;break;
        }
    }
    return true;
}

bool Decoder::ctrl_MMOC_1(int difference_of_pic_nums_minus1)
{
    int picNumX  = cur_slice->ps->CurrPicNum - (difference_of_pic_nums_minus1 + 1);
    std::vector<picture*>::iterator it;
    it = std::find_if(list_Ref.begin(), list_Ref.end(),\
    [picNumX](picture* i)->bool{return i && i->is_UsedForShort() && i->PicNum == picNumX;}\
    );
    if(*it == NULL) return false;
    else (*it)->set_NotUseForRef();
    return true;
}
bool Decoder::ctrl_MMOC_2(int long_term_pic_num)
{
    std::vector<picture*>::iterator it;
    it = std::find_if(list_Ref.begin(), list_Ref.end(),\
    [long_term_pic_num](picture* i)->bool{return i && i->is_UsedForLong() && i->LongTermPicNum == long_term_pic_num;}\
    );
    if(*it == NULL) return false;
    else 
    {
        (*it)->set_NotUseForRef();
        return true;
    }
}
bool Decoder::ctrl_MMOC_3(int difference_of_pic_nums_minus1,int long_term_frame_idx)
{
    int picNumX = cur_slice->ps->CurrPicNum - (difference_of_pic_nums_minus1 + 1);
    std::vector<picture*>::iterator it;
    it = std::find_if(list_Ref.begin(), list_Ref.end(),\
    [picNumX](picture* i)->bool{return i && i->is_UsedForShort() && i->PicNum == picNumX;}\
    );
    if(*it == NULL) return false;
    else 
    {
        (*it)->set_UseForLong(long_term_frame_idx);
        return true;
    }
}
bool Decoder::ctrl_MMOC_4(int max_long_term_frame_idx_plus1)
{
    if(max_long_term_frame_idx_plus1 == 0)  MaxLongTermFrameIdx = -1;
    else MaxLongTermFrameIdx = max_long_term_frame_idx_plus1 - 1;
    return true;
}
bool Decoder::ctrl_MMOC_5()   
{
    MaxLongTermFrameIdx = -1;
    //全部标记为不用于参考，但是在下一次初始化的时候就会全部删除，所以这里直接全部清空
    list_Ref.clear();
    list_Ref0.clear();
    list_Ref1.clear();
    return true;
}
bool Decoder::ctrl_MMOC_6(int long_term_frame_idx)
{
    pic_current->set_UseForLong(long_term_frame_idx);
    pic_current->memory_management_control_operation = 6;
    // LongTermFrameIdx 在下一次初始化的时候会计算到
    return true;
}
bool Decoder::ctrl_FIFO(int max_num_ref_frames)
{//如果短期参考帧的数量达到最大参考帧数量减去长期帧的那么移除最小的 FrameNumWrap 的 pic 标记为不用于参考
    //先进先出，所以从前往后遍历到第一个短期参考帧就行
    if(list_Ref_short.size() == max_num_ref_frames - list_Ref_long.size())
    {
        std::vector<picture*>::iterator it;
        it = std::find_if(list_Ref.begin(), list_Ref.end(), [](picture* i)->bool{return i->is_UsedForShort();});
        (*it)->set_NotUseForRef();
    }
    else
    {
        int a = 0;
    }
    return true;
}
Decoder::Decoder()
{
    matrix_4x4Trans = new matrix(4,4,0);
    matrix_2x2Trans = new matrix(2,2,0);
    int t4[16] = 
    {
        1,  1,  1,  1,
        1,  1, -1, -1, 
        1, -1, -1,  1,
        1, -1,  1, -1
    };
    (*matrix_4x4Trans).read_from(t4);
    int t2[4] =
    {
        1, 1,
        1,-1
    };
    (*matrix_2x2Trans).read_from(t2);
    pic_current = NULL;
    pic_uprefer = NULL;
    MaxLongTermFrameIdx = 100;
}
Decoder::~Decoder()
{
    delete matrix_4x4Trans;
    delete matrix_2x2Trans;
}