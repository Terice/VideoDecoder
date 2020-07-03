#include "Decoder.h"


#include "matrix.h"
#include "picture.h"

#include <vector>
void Decoder::print_list()
{
    printf(">>deder:    (direction:----->)  [Frame_num](dec_num)\n");
    printf("         deocded list :");
    for (uint8_t i = 0; i < list_Decoded.size(); i++)
    {
        printf("%d(%d), ",list_Decoded[i]->FrameNum, list_Decoded[i]->DecNum);
    }
    printf("\n");
    // printf("         reflist   list :");
    // for (uint8_t i = 0; i < list_Ref.size(); i++)
    // {
    //     printf("%d(%d), ",list_Ref[i]->FrameNum, list_Ref[i]->DecNum);
    // }
    // printf("\n");
    printf("         reflist_0 list :");
    for (uint8_t i = 0; i < list_Ref0.size(); i++)
    {
        printf("%d(%d), ",list_Ref0[i]->FrameNum, list_Ref0[i]->DecNum);
    }
    printf("\n");
    printf("         reflist_1 list :");
    for (uint8_t i = 0; i < list_Ref1.size(); i++)
    {
        printf("%d(%d), ",list_Ref1[i]->FrameNum, list_Ref1[i]->DecNum);
    }
    printf("\n");
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
    pic_uprefer = NULL;
    return true;
};

//fw的内存控制
bool Decoder::ctrl_Memory()
{
    int tmp = 0;
    if(list_Ref0.size() == 6)
    {
        //首先删除表0的前两个元素
        list_Ref0.erase(list_Ref0.begin());
        tmp = (!(list_Ref0[0]->is_IDR()))?list_Ref0[0]->DecNum:-1;
        list_Ref0.erase(list_Ref0.begin());
        for(std::vector<picture*>::iterator it = list_Decoded.begin(); it != list_Decoded.end(); it++)
        {
            if((*it)->DecNum == tmp && !(*it)->is_IDR())
            Sdelete_s(*it);
        }
        uint8_t state = 0;
        for (std::vector<picture*>::iterator it = list_Decoded.end() - 1; it != list_Decoded.begin() - 1; it--)
        {
            if((*it) == NULL){state = 1;}
            if(state == 1)
            {
                if(!(*it) || ((*it) && !(*it)->is_IDR()))
                {
                    Sdelete_s(*it);
                    list_Decoded.erase(it);
                }
            }
        }
    }
    
    return true;
}
//fw的排序
bool Decoder::init_RefPicList(Slicetype type)
{
    //如果当前是P帧，那么缓冲区前一帧入l0，同时把当前帧赋给一个临时变量
    if(type == P || type == SP)
    {
        list_Ref1.clear();
        //前向参考，参考帧的帧号必定小于当前帧的帧号
        if(pic_uprefer != NULL)
            list_Ref0.push_back(pic_uprefer);
        else 
            list_Ref0.push_back(list_Ref.back());
        //然后排序

        //

        //下一次遇到P帧的时候这个P帧入列表0
        pic_uprefer = get_CurrentPic();
        return true;
    }
    //如果是B帧，参考序列的最后一帧入l1
    else //if(type == B)
    {
        
        list_Ref1.push_back(*(list_Ref0.end() - 2));

        //分别排序
        return false;
    }
}
bool Decoder::calc_PictureNum(int MaxFrameNum, int field_pic_flag)
{
    for (int i = 0; i < list_Ref0.size(); i++)
    {
        picture* tmp = list_Ref0[i];
        if(tmp->is_UsedForShort())
        {
            //这里本来应该是和frame_num句法元素比较，但是在这一步之前已经把pic加入到解码队列中了，并且把句法元素赋值给了FrameNum
            if(tmp->FrameNum > pic_current->FrameNum)
                tmp->FrameNumWrap = tmp->FrameNum - MaxFrameNum;
            else tmp->FrameNumWrap = tmp->FrameNum;
        }
        
        if(!field_pic_flag)
        {
            if(tmp->is_UsedForShort())
            {tmp->PicNum = tmp->FrameNumWrap;}
            if(tmp->is_UsedForLong())
            {tmp->LongTermPicNum = tmp->LongTermFrameIdx;}
        }
    }
    return true;
}
Decoder::~Decoder()
{
    delete matrix_4x4Trans;
    delete matrix_2x2Trans;
}