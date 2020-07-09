
#include "functions.h"
#include "residual.h"
#include "slice.h"
#include "macroblock.h"
#include "block.h"
#include "Parser.h"
#include "Debug.h"
#include "array2d.h"
#include <cmath>
#include "staticcharts.h"

#include <iostream>
#include "Decoder.h"
#include "matrix.h"

#include "Debug.h"


//根据解析的模式进行解析
void residual::Parse(uint8_t index)
{
    //4x4最后一次索引是15，出来了就解亮度，
    switch (mb->premode)
    {
        case Intra_16x16:Parser_Intra16x16();break;
        case Intra_4x4:Parser_Intra4x4(index);;break;
        case Intra_8x8:Parser_Intra8x8(index);break;
        default:Parser_Inter();break;
    }
    Parser_Chroma();
}
void residual::Parser_Inter()
{
    //帧间残差也是4x4的解析方法
    luma = new block*[1];
    luma[0] = new block();
    luma[0]->add_childBlock(4);
    for(uint8_t i8x8 = 0; i8x8 < 4; i8x8++)
    {
        luma[0]->get_childBlock(i8x8)->add_childBlock(4, 16);
        for(uint8_t i4x4 = 0; i4x4 < 4; i4x4++) 
        {
            if(codedPatternLuma & (1 << i8x8)) (this->*residual_block)(\
            luma[0]->get_childBlock(i8x8)->get_childBlock(i4x4), 0x230 + i8x8 * 4 + i4x4, 0, 15, 16);
            else for(uint8_t i = 0; i < 16; i++) luma[0]->get_childBlock(i8x8)->get_childBlock(i4x4)->set_blockValue(i, 0);
        }
    }
}
void residual::Parser_Intra16x16()
{
    luma = new block*[2];
    //parse luma
    //DC
    luma[1] = new block(16);
    (this->*residual_block)(luma[1], 0x030,  0, 15, 16);
    //AC
    luma[0] = new block();
    luma[0]->add_childBlock(4);
    for(uint8_t i8x8 = 0; i8x8 < 4; i8x8++)
    {
        luma[0]->get_childBlock(i8x8)->add_childBlock(4, 15);
        for(uint8_t i4x4 = 0; i4x4 < 4; i4x4++)
        {
            if(i8x8 == 2 && i4x4 == 0)
                int a = 0;
            if(codedPatternLuma & (1 << i8x8)) (this->*residual_block)(luma[0]->get_childBlock(i8x8)->get_childBlock(i4x4), 0x130 + i8x8 * 4 + i4x4, 0, 14, 15 );
            else for(uint8_t i = 0; i < 15; i++)luma[0]->get_childBlock(i8x8)->get_childBlock(i4x4)->set_blockValue(i, 0);
        }
    }
}
void residual::Parser_Intra4x4(int index)
{
    //一次解完所有的256个变换系数
    luma = new block*[1];
    luma[0] = new block();
    luma[0]->add_childBlock(4);
    for(uint8_t i8x8 = 0; i8x8 < 4; i8x8++)
    {
        luma[0]->get_childBlock(i8x8)->add_childBlock(4, 16);
        for(uint8_t i4x4 = 0; i4x4 < 4; i4x4++) 
        {
            if(codedPatternLuma & (1 << i8x8)) (this->*residual_block)(\
            luma[0]->get_childBlock(i8x8)->get_childBlock(i4x4), 0x230 + i8x8 * 4 + i4x4, 0, 15, 16);
            else for(uint8_t i = 0; i < 16; i++) luma[0]->get_childBlock(i8x8)->get_childBlock(i4x4)->set_blockValue(i, 0);
        }
    }
}
void residual::Parser_Intra8x8(int index)
{
    luma = new block*[1];
    luma[0] = new block();luma[0]->add_childBlock(16, 16);
    for(uint8_t i8x8 = 0; i8x8 < 4; i8x8++)
        if(codedPatternLuma & (1 << i8x8))(this->*residual_block)(luma[0]->get_childBlock(i8x8),0x530 + i8x8, 0, 63, 64);
        else for(uint8_t i = 0; i < 64; i++) luma[0]->get_childBlock(i8x8)->set_blockValue(i, 0);
}

void residual::Parser_Chroma()
{
    if(mb->position_x == 14 && mb->position_y == 52)
        int a = 0;
    //parser chroma
    //0 - DC 1 - AC
    chroma = new block*[2];
    chroma[0] = new block();chroma[0]->add_childBlock(2, 4);   //DC
    chroma[1] = new block();
    if(parser->pV->ChromaArrayType == 1 || parser->pV->ChromaArrayType == 2)
    {
        uint8_t iCbCr = 0;
        uint8_t NumC8x8 = 4 / (parser->pV->SubWidthC * parser->pV->SubHeightC);

        
        for(iCbCr = 0; iCbCr < 2; iCbCr++) // read chroma DC level
            if(codedPatternchroma & 3) 
                (this->*residual_block)(chroma[0]->get_childBlock(iCbCr), 0x300 + ((int)iCbCr << 4), 0, 4 * NumC8x8 - 1, 4 * NumC8x8);
            else for(uint8_t i = 0; i < 4 * NumC8x8; i++) chroma[0]->get_childBlock(iCbCr)->set_blockValue(i, 0);
        chroma[1]->add_childBlock(2);
        for(iCbCr = 0; iCbCr < 2; iCbCr++) // read chroma AC level
        {
            for(uint8_t i8x8 = 0; i8x8 < NumC8x8; i8x8++)
            {
                chroma[1]->get_childBlock(iCbCr)->add_childBlock(4, 15);
                for(uint8_t i4x4 = 0; i4x4 < 4; i4x4++)
                    if(codedPatternchroma & 2) (this->*residual_block)(chroma[1]->get_childBlock(iCbCr)->get_childBlock(i4x4), 0x400 + ((int)iCbCr << 4) + i8x8 * 4 + i4x4, 0, 14, 15);
                    else for(uint8_t i = 0; i < 15; i++) chroma[1]->get_childBlock(iCbCr)->get_childBlock(i4x4)->set_blockValue(i, 0);
            }
        }
    }
}


//数组变换为矩阵的变换表
//变换矩阵：输入的两个索引分别是
//1   在数组中的索引
//2   0代表row 1代表col
static uint8_t Inverse4x4ZigZag[16][2] = {
    {0,0}, {0,1}, {1,0}, {2,0}, 
    {1,1}, {0,2}, {0,3}, {1,2}, 
    {2,1}, {3,0}, {3,1}, {2,2}, 
    {1,3}, {2,3}, {3,2}, {3,3}
};

//把一个数组[16]变换为一个4x4矩阵并写入到第二个参数的矩阵中
//第一个参数是要变换的数组(int[16])，第二个参数是目标矩阵(4x4)
void Inverse4x4Scan(int* res, matrix& dst)
{
    int *cur = NULL;
    //如果数据来自同一个矩阵
    if(res == dst.data)
    {
        cur = new int[16]();
        for(uint8_t i = 0; i < 16; i++)
        {cur[i] = res[i];}
    }
    else //不是同一个矩阵的元素
    {cur = res;}//将res指针给cur
    
    uint8_t r = 0, c = 0;
    for (uint8_t i = 0; i < 16; i++)
    {
        r = Inverse4x4ZigZag[i][0];
        c = Inverse4x4ZigZag[i][1];
        dst[r][c] = cur[i];
    }
    //如果来自同一矩阵，那么释放内存
    if(res == dst.data) Sdelete_l(cur);
    //
}
uint8_t v[6][3] = {
        {10, 16, 13},    
        {11, 18, 14},
        {13, 20, 16},
        {14, 23, 18},
        {16, 25, 20},
        {18, 29, 23}
    };
int LevelScale4x4(int parameter, int i, int j)
{
    if(i % 2 == 0 && j % 2 ==0) return 16*v[parameter][0];
    else if(i % 2 == 1 && j % 2 ==1) return 16*v[parameter][1];
    else return 16*v[parameter][2];
}
void residual::Decode(uint8_t index)
{
    
    if(mb->is_intrapred())
        switch (mb->premode)
        {
            case Intra_16x16:Decode_Intra16x16(); break;
            case Intra_4x4:Decode_Intra4x4(); break;
            default: break;
        }
    else //if(mb->is_interpred())
    {
        Decode_Intra4x4();
    }
    Decode_Chroma(0);
    Decode_Chroma(1);

}
void residual::Decode_Chroma(int iCbCr)
{
    //取得当前的色度量化参数
    qP = mb->QPC_;
    matrix* c = new matrix(2,2,0);
    for (uint8_t i = 0; i < 4; i++) (*c).get_value_i(i) = (*chroma[0])[iCbCr]->value[i];
    

    if(parser->debug->residual_transcoeff())
    cout <<">>decode: decode residual coef  chroma: " << iCbCr << endl << "       input DC:" << endl << (*c) << endl;


    (*c)  = (*decoder->matrix_2x2Trans * (*c)  * *decoder->matrix_2x2Trans);
    (*c)  = ((((*c)  * LevelScale4x4(qP % 6, 0, 0)) << (qP / 6)) >> 5);
    matrix* dcC = new matrix(2, 2);
    (*dcC) << (*c) ;
    delete c;
    
    array2d<matrix*>* lumaResidual = new array2d<matrix*>(2,2,NULL);
    for (uint8_t i = 0; i < 4; i++)
    {lumaResidual->get_value_i(i) = new matrix(4,4,1);}
    
    for (uint8_t i = 0; i < 4; i++)
    {
        auto des = lumaResidual->get_value_i(i);
        (*des)[0][0] = (*dcC).get_value_i(i);
        // cout << (*des) << endl;
        for (uint j = 0; j < 15; j++)
        //block类的[]运算符是取到子box而不是值value
        {(*des).get_value_i(j + 1) = (*(*chroma[1])[iCbCr])[i]->value[j];}

        Inverse4x4Scan((*des).data, (*des));
        (*des) = ScalingAndTransform_Residual4x4Blocks(8, this->qP, des, 1);

    }
    delete dcC;

    //wtire chroma into out matrix
    matrix* result = new matrix(8,8,1);
    for (uint8_t i = 0; i < 8; i++)
    {
        for (uint8_t j = 0; j < 8; j++)
        {
           (*result)[i][j] = (*(*lumaResidual)[i/4][j/4])[i%4][j%4];
        }
    }
    for (uint8_t i = 0; i < 4; i++){delete lumaResidual->get_value_i(i);}
    delete lumaResidual;


    if(iCbCr == 0) residual_U = result;
    else residual_V = result;

    if(parser->debug->residual_transcoeff()) cout << "      output:" << endl << (*result) << endl;
}


void residual::Decode_Intra4x4()
{
    qP = mb->QPY_;
    residual_Y = new matrix(16,16,0);
    for (uint8_t i8x8 = 0; i8x8 < 4; i8x8++)
    {
        for (uint8_t i4x4 = 0; i4x4 < 4; i4x4++)
        {
            matrix c(4,4,0);
            Inverse4x4Scan(luma[0]->get_childBlock(i8x8)->get_childBlock(i4x4)->value, c);
            ScalingAndTransform_Residual4x4Blocks(parser->pV->BitDepthY, qP, &c);
            int row_cur = 0, col_cur = 0;
            mb->get_PosByIndex(4 * i8x8 + i4x4, row_cur, col_cur);
            // cout << "i8x8" << (int)i8x8 << " i4x4" << (int)i4x4 << endl << "row " << row_cur << "col " << col_cur << endl << c << endl;
            //当前4x4块写入，一共写入16次，256个样点
            for (uint8_t row = 0; row < 4; row++)
            {
                for (uint8_t col = 0; col < 4; col++)
                {
                    (*residual_Y)[row_cur + row][col_cur + col] = c[row][col];
                }
            }
        }
    }
    // cout << (*(mb->pred_Y)) << endl;
}

void residual::Decode_Intra16x16()
{

    qP = mb->QPY_;

    matrix c(4, 4, 0);
    Inverse4x4Scan(luma[1]->value, c);



    //Intra_16x16 DC transform and scaling

    // c  = (*decoder->matrix_4x4Trans * c  * *decoder->matrix_4x4Trans);
    matrix tmp(4,4,0);
    tmp += *decoder->matrix_4x4Trans;
    tmp *= c;
    tmp *= *decoder->matrix_4x4Trans;
    c << tmp;
    // //上面的优化对速度没有多少提高

    //原本使用符号重载的抽象公式，但是每次都需要值拷贝两次，所以拆开成多个公式，直接在左值做修改，
    if(qP >= 36)
    {
        // c  = ((c  * LevelScale4x4(qP % 6, 0, 0)) << (qP / 6 - 6));
        c *= LevelScale4x4(qP % 6, 0, 0);
        c <<= (qP / 6 - 6);
    }
    else 
    {
        //c  = (((c  * LevelScale4x4(qP % 6, 0, 0)) + (1 << (5 - qP /6))) >> (6 - qP / 6));
        c *= LevelScale4x4(qP % 6, 0, 0);
        c += (1 << (5 - qP /6));
        c >>= (6 - qP / 6);
    }

    if(parser->debug->residual_transcoeff()) cout << c<< endl;

    matrix dcY(4,4);
    dcY << c;//直接将c的数据交给dcY


    //4x4个4x4矩阵
    array2d<matrix*> lumaResidual(4, 4, NULL);
    for (uint8_t i = 0; i < 16; i++)
    {lumaResidual.get_value_i(i) = new matrix(4,4,0);}
    
    for (uint8_t i = 0; i < 16; i++)
    {
        matrix* des = lumaResidual.get_value_i(i);
        // cout << "des: " << des << endl;
        //write DC into residual
        (*des)[0][0] = dcY.get_value_i(i);
        //write AC into residual

        //提前进行了一次预处理得到luma的block(AC系数所在的block)
        block* bl = luma[0]->get_childBlock(block4x4Index[i]/4)->get_childBlock(block4x4Index[i]%4);
        //填入AC系数的时候，AC系数不是按扫描顺序填入DC之后的位置的。而是按照4x4逆扫描。
        //也就是说：读取AC的值的时候按逆扫描来，填入的时候按照光栅扫描填入
        for(uint8_t j = 0; j < 15; j++)
        {(*des).get_value_i(j + 1) = bl->value[j];}
        //inverse scan AC+DC coefficents
        Inverse4x4Scan((*des).data, (*des));
        ScalingAndTransform_Residual4x4Blocks(8, this->qP, des, 1);
        // cout << "i : " << (int)i << endl << (*des) << endl;
    }
    

    //4x4个4x4矩阵写入到一个16x16矩阵中去
    matrix* result = new matrix(16, 16, 1);
    for (uint8_t i = 0; i < 16; i++)
    {
        for (uint8_t j = 0; j < 16; j++)
        {
            (*result)[i][j] = (*lumaResidual[i/4][j/4])[i%4][j%4];
        }
    }
    for (uint8_t i = 0; i < 16; i++){delete lumaResidual.get_value_i(i);}

    residual_Y = result;
}
//缩放变换一个4x4矩阵
matrix& residual::ScalingAndTransform_Residual4x4Blocks(int BitDepth, int qP, matrix* c, uint8_t mode)
{
    uint8_t i = 0, j = 0;
    //d_ij
    matrix d(4, 4, 0);
    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if(qP >= 24) d[i][j] = ((*c)[i][j] * LevelScale4x4(qP % 6, i, j)) << (qP / 6 - 4);
            else 
            {
                d[i][j] = ((*c)[i][j] * LevelScale4x4(qP % 6, i, j) + (int)powl(2, 3 - qP / 6)) >> (4 - qP / 6);
            }
        }
    }
    if(mode == 1) d[0][0] = (*c)[0][0];
    //e_ij
    matrix e(4,4,0);
    for(i = 0; i < 4; i++) {e[i][0] = d[i][0] + d[i][2];}
    for(i = 0; i < 4; i++) {e[i][1] = d[i][0] - d[i][2];}
    for(i = 0; i < 4; i++) {e[i][2] =(d[i][1] >> 1 )- d[i][3];}
    for(i = 0; i < 4; i++) {e[i][3] = d[i][1] + (d[i][3] >> 1);}

    //f_ij
    matrix f(4,4);
    f << d;//不需要的空间重新利用
    for(i = 0; i < 4; i++) {f[i][0] = e[i][0] + e[i][3];}
    for(i = 0; i < 4; i++) {f[i][1] = e[i][1] + e[i][2];}
    for(i = 0; i < 4; i++) {f[i][2] = e[i][1] - e[i][2];}
    for(i = 0; i < 4; i++) {f[i][3] = e[i][0] - e[i][3];}

    //g_ij
    matrix g(4,4);
    g << e;//不需要的空间重新利用
    for(j = 0; j < 4; j++) {g[0][j] = f[0][j] + f[2][j];}
    for(j = 0; j < 4; j++) {g[1][j] = f[0][j] - f[2][j];}
    for(j = 0; j < 4; j++) {g[2][j] =(f[1][j] >> 1) - f[3][j];}
    for(j = 0; j < 4; j++) {g[3][j] = f[1][j] + (f[3][j] >> 1);}

    //h_ij
    matrix h(4,4);
    h << f;//不需要的空间重新利用
    for(j = 0; j < 4; j++) {h[0][j] = g[0][j] + g[3][j];}
    for(j = 0; j < 4; j++) {h[1][j] = g[1][j] + g[2][j];}
    for(j = 0; j < 4; j++) {h[2][j] = g[1][j] - g[2][j];}
    for(j = 0; j < 4; j++) {h[3][j] = g[0][j] - g[3][j];}
    //这个if已经无法使用
    // if(0)
    // {
    //     cout << ">>residual: d e f  g h:-----------" << endl;
    //     cout << d << endl;
    //     cout << e << endl;
    //     cout << f << endl;
    //     cout << g << endl;
    //     cout << h << endl;
    //     cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << endl;
    // }
    //r_ij
    matrix r(4,4);
    //r = ((h + 32) >> 6);
    r << h;//h的结果直接交给r并在r上运算
    r += 32;
    r >>= 6;
    
    (*c) << r;
    return (*c);
}
void residual::Calc()
{


}
void residual::ZeroAll()
{
    //块置零
    //数据置零
    if(mb->is_intrapred())
    {
        if(mb->premode == Intra_16x16)
        {
            luma = new block*[2];
            luma[1] = new block(16);
            luma[0] = new block();
            luma[0]->add_childBlock(4);
            for (uint8_t i = 0; i < 4; i++){luma[0]->get_childBlock(i)->add_childBlock(4, 0);}

        }
        else if(mb->premode == Intra_4x4)
        {
            luma = new block*[1];
            luma[0] = new block();
            luma[0]->add_childBlock(4);
            for (uint8_t i = 0; i < 4; i++){luma[0]->get_childBlock(i)->add_childBlock(4, 0);}
        }
        else {
            int a = 0;/*这里是8x8块8*/
        }
    }
    else
    {
        luma = new block*[1];
        luma[0] = new block();
        luma[0]->add_childBlock(4);
        for (uint8_t i = 0; i < 4; i++){luma[0]->get_childBlock(i)->add_childBlock(4, 0);}
    }
    

    //亮度的块置零
    chroma = new block*[2];
    chroma[0] = new block();
    chroma[0]->add_childBlock(2, 4);
    chroma[1] = new block();
    chroma[1]->add_childBlock(2);
    for(uint8_t iCbCr = 0; iCbCr < 2; iCbCr++){chroma[1]->add_childBlock(4,0);}

    //样点残差块置零
    residual_Y = new matrix(16,16,0);
    residual_U = new matrix(8,8,0);
    residual_V = new matrix(8,8,0);
}
void residual::residual_block_cavlc(block* bl, int syntaxValue, uint8_t startIdx, uint8_t endIdx, uint8_t length)
{
    // uint8_t i = 0;
    // uint32_t coeff_token = 0;
    // uint8_t suffixLength = 0;
    // int16_t levelCode = 0;
    // uint8_t zerosLeft;

    // int16_t trailing_ones_sign_flag;

    // for(i = 0; i < length; i++) 
    // coeffLevel[i] = 0;
    // coeff_token = parser_g->read_ce();

    // int32_t* levelVal = new int32_t[Get_TotalCoeff(coeff_token)];
    // if(Get_TotalCoeff(coeff_token) > 0)
    // {
    //     if(Get_TotalCoeff(coeff_token) > 10 && Get_TrailingOnes(coeff_token) < 3)  suffixLength = 1;
    //     else suffixLength = 0;
    //     for(i = 0; i < Get_TotalCoeff(coeff_token); i++)
    //     {
    //         if(i < Get_TrailingOnes(coeff_token))
    //         { 
    //             trailing_ones_sign_flag = parser_g->read_ce();
    //             levelVal[i] = 1 - 2 * trailing_ones_sign_flag;
    //         }
    //         else 
    //         { 
    //             level_prefix = parser_g->read_ce();
    //                                                                                         levelCode = (((15 <= level_prefix) ? 15 : level_prefix) << suffixLength);
    //             if(suffixLength > 0 || level_prefix >= 14)
    //             {
    //                 level_suffix = parser_g->read_un(suffixLength);
    //                                                                                         levelCode += level_suffix;
    //             }
    //             if(level_prefix >= 15 && suffixLength == 0)                                 levelCode += 15;
    //             if(level_prefix >= 16)                                                      levelCode += (1 << (level_prefix - 3)) - 4096;
    //             if(i == Get_TrailingOnes(coeff_token) && Get_TrailingOnes(coeff_token) < 3) levelCode += 2;
    //             if(levelCode % 2 == 0)                                                      levelVal[i] = (levelCode + 2) >> 1;
    //             else                                                                        levelVal[i] = (-levelCode - 1) >> 1;
    //             if(suffixLength == 0)                                                       suffixLength = 1;
    //             if(abs(levelVal[i]) > (3 << (suffixLength - 1)) && suffixLength < 6)        suffixLength++;
    //         }
    //     }
    //     if(Get_TotalCoeff(coeff_token) < endIdx - startIdx + 1)
    //     {
    //         total_zeros  = parser_g->read_ce();
    //         zerosLeft = total_zeros;
    //     }
    //     else zerosLeft = 0;
    //     uint16_t* runVal = new uint16_t[Get_TotalCoeff(coeff_token)];
    //     for(i = 0; i < Get_TotalCoeff(coeff_token) - 1; i++)
    //     { 
    //         if(zerosLeft > 0)
    //         {
    //             run_before = parser_g->read_ce();
    //             runVal[i] = run_before;
    //         }
    //         else runVal[i] = 0;
    //         zerosLeft = zerosLeft - runVal[i];
    //     } 
    //     runVal[Get_TotalCoeff(coeff_token) - 1] = zerosLeft ;
    //     int8_t coeffNum;
    //     coeffNum = -1 ;
    //     for(i = Get_TotalCoeff(coeff_token) - 1; i >= 0; i--) 
    //     { 
    //         coeffNum += runVal[i] + 1 ;
    //         coeffLevel[startIdx + coeffNum] = levelVal[i] ;
    //     }
    //     delete[] runVal;
    // }
    // delete[] levelVal;
}
void residual::residual_block_cabac(block* bl, int requestVlaue, uint8_t startIdx, uint8_t endIdx, uint8_t length)
{
    int16_t i = 0, numCoeff = 0;
    int32_t coeffLevel                  [length];
    uint8_t significant_coeff_flag      [length];
    uint8_t last_significant_coeff_flag [length];
    uint32_t a= 0 ,b = 0;

    if(length != 64 || parser->pV->ChromaArrayType == 3) 
        bl->coded_block_flag = parser->read_ae(0x61000 + requestVlaue);
    else bl->coded_block_flag = 1;
    for(i = 0; i < length; i++) 
        coeffLevel[i] = 0;
    if(bl->coded_block_flag)
    {
        int32_t coeff_abs_level_minus1  [length];
        int32_t coeff_sign_flag         [length];
        numCoeff = endIdx + 1 ;
        i = startIdx ;
        while(i < numCoeff - 1) 
        {
            significant_coeff_flag[i] = parser->read_ae(0x62000 + requestVlaue + (((int32_t)i) << 20));
            if(significant_coeff_flag[i]) {
                last_significant_coeff_flag[i] = parser->read_ae(0x63000 + requestVlaue + (((int32_t)i) << 20));  
                if(last_significant_coeff_flag[i]) 
                    numCoeff = i + 1;
            }
            i++;
        }
        coeff_abs_level_minus1[numCoeff - 1] = parser->read_ae(0x64000 + requestVlaue + (((int)a) << 24) + (((int)b )<< 20));
        coeff_sign_flag[numCoeff - 1]        = parser->read_ae(65);//use bypass decode
        coeffLevel[numCoeff - 1] = (coeff_abs_level_minus1[numCoeff - 1] + 1) * (1 - 2 * coeff_sign_flag[numCoeff - 1]) ;
        if(coeffLevel[numCoeff - 1] == 1 || coeffLevel[numCoeff - 1] == -1)a++;
        if(coeffLevel[numCoeff - 1] > 1  || coeffLevel[numCoeff - 1] <  -1)b++;
        for(i = numCoeff - 2; i >= startIdx; i--){
            if(significant_coeff_flag[i])
            { 
                coeff_abs_level_minus1[i] = parser->read_ae(0x64000 + requestVlaue + (((int)a) << 24) + (((int)b )<< 20));  
                coeff_sign_flag[i]        = parser->read_ae(65);  
                coeffLevel[i] = (coeff_abs_level_minus1[i] + 1) * (1 - 2 * coeff_sign_flag[i]);

                if(coeffLevel[i] == 1 || coeffLevel[i] == -1)a++;
                if(coeffLevel[i] > 1  || coeffLevel[i] <  -1)b++;
            }
        }
        // delete[](int32_t*) coeff_abs_level_minus1; coeff_abs_level_minus1 = NULL;
        // delete[](int32_t*) coeff_sign_flag       ; coeff_sign_flag = NULL;
    }
    for (uint16_t i = 0; i < length; i++) {bl->set_blockValue(i, coeffLevel[i]);}

    if(debug->cabac_result_residual())
    {
        printf(">>residu:result of cabac is: \n");
        for (uint16_t i = 0; i < length; i++) {printf(" index %2d, value : %d\n", i, coeffLevel[i]);}
    }

    // delete[] coeffLevel                 ;
    // delete[] significant_coeff_flag     ;
    // delete[] last_significant_coeff_flag;
}
residual::residual(macroblock* ma, Parser* pa)
{
    this->decoder = ma->up_slice->decoder;
    this->debug = pa->debug;
    codedPatternLuma = ma->CodedBlockPatternLuma;
    codedPatternchroma = ma->CodedBlockPatternChroma;
    this->TransformBypassModeFlag = ma->TransformBypassModeFlag;
    mb = ma;
    parser = pa;
    mode = 0;
    cabacFlag = pa->pS->pps->entropy_coding_mode_flag;
    if(!cabacFlag) residual_block = &residual::residual_block_cavlc;
    else residual_block = &residual::residual_block_cabac;
}
residual::~residual()
{
    if(mb->premode == Intra_16x16)
    {
        Sdelete_s(luma[1]);
        Sdelete_s(luma[0]);
        Sdelete_l(luma);

    }
    else if(mb->premode == Intra_4x4 || mb->is_interpred())
    {
        Sdelete_s(luma[0]);
        Sdelete_l(luma);
    }
    else {int a = 0;/*这里是8x8块8*/}

    Sdelete_s(chroma[0]);
    Sdelete_s(chroma[1]);
    Sdelete_l(chroma);
    Sdelete_s(residual_Y);
    Sdelete_s(residual_U);
    Sdelete_s(residual_V);
}