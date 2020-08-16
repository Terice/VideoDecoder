#include "matrix.h"
#include "Debug.h"
#if DE_MATRIX_RESULT
#include <iostream>
using namespace std;
#endif


matrix& operator>>=(matrix& left,int right)
{
    for (size_t i = 0; i < left.x_length; i++)
    {
        for (size_t j = 0; j < left.y_length; j++)
        {
            left[i][j] >>= right;
        }
    }
    return left;
}
matrix& operator<<=(matrix& left,int right)
{
    for (size_t i = 0; i < left.x_length; i++)
    {
        for (size_t j = 0; j < left.y_length; j++)
        {
            left[i][j] <<= right;
        }
    }
    return left;
}
matrix& matrix::operator+=(int right)
{
    for (size_t i = 0; i < x_length * y_length; i++)
    {
        this->get_value_i(i) += right;
    }
    return (*this);
}
matrix& matrix::operator*=(int right)
{
    for (size_t i = 0; i < x_length * y_length; i++)
    {
        this->get_value_i(i) *= right;
    }
    return (*this);
}
matrix& matrix::operator/=(int right)
{
    for (size_t i = 0; i < x_length * y_length; i++)
    {
        this->get_value_i(i) /= right;
    }
    return (*this);
}
matrix& matrix::operator+=(const matrix& right)
{
    if(this->x_length != right.x_length || this->y_length != right.y_length) return (*this);
    else 
    {
        for (size_t i = 0; i < x_length * y_length; i++)
        {this->get_value_i(i) += right.get_value_i(i);}
        return (*this);
    }
}
bool operator>(array2d<int>& dst, matrix& ma)
{
    if(dst.data == NULL) return false;
    else 
    {
        if(ma.data != NULL) ma.delete_data();
        ma.data = dst.data; 
        dst.delete_data();
    }
    return true;
}
bool operator<(array2d<int>& dst, matrix& ma)
{
    if(ma.data == NULL) return false;
    else 
    {
        if(dst.data != NULL) dst.delete_data();
        dst.data = ma.data; 
        ma.delete_data();
    }
    return true;
}
std::ostream& operator<<(std::ostream& out , const matrix& ma)
{
    for (size_t r = 0; r < ma.x_length; r++)
    {
        if(r % 4 == 0) 
        {
            printf("%-2zu", r);
            for(size_t i = 0; i < ma.y_length; i+=4) {printf("---------------------------%-2lu", i/4 + 1);}
            printf("\n");
        }
        for (size_t c = 0; c < ma.y_length; c++)
        {
            if(c % 4 == 0) out << "|";
            printf("%6d ", ma[r][c]);
        }
        printf("|\n");
    }
    printf("%-2d", ma.x_length);
    for(size_t i = 0; i < ma.y_length; i+=4) {printf("---------------------------%-2lu", i/4 + 1);}
    
    return out;
}

matrix operator+(const matrix& left, int right)
{
    matrix result(left.x_length, left.y_length, 0);
    for (size_t i = 0; i < left.x_length; i++)
    {
        for (size_t j = 0; j < left.y_length; j++)
        {
            result[i][j] = left[i][j] + right;
        }
    }
    return result;
}


matrix& matrix::operator*=(matrix& right)
{
    matrix& left = (*this);
    int r = left.x_length, c = right.y_length;
    matrix result(r, c, 0);
    if(left.y_length != right.x_length) return left;
    else 
    { 
        int result_cur = 0;
        for(size_t r_cur = 0; r_cur < r; r_cur++)
        {
            for (uint8_t cur = 0; cur < right.x_length; cur++)
            {
                result_cur = left[r_cur][cur];
                for(size_t c_cur = 0; c_cur < c; c_cur++)
                {
                    result[r_cur][c_cur] += result_cur * right[cur][c_cur];
                }
            }
        }

        left << result;
        
        return left;
    }
}
matrix operator*(const matrix& left, const matrix& right)
{
    int r = left.x_length, c = right.y_length;
    matrix result(r, c, 0);
    if(left.y_length != right.x_length) return left;
    else 
    { 
        int result_cur = 0;
        for(size_t r_cur = 0; r_cur < r; r_cur++)
        {
            for(size_t c_cur = 0; c_cur < c; c_cur++)
            {
                for (uint8_t cur = 0; cur < right.x_length; cur++)
                {
                    result_cur = left[r_cur][cur] * right[cur][c_cur];
                }
                result[r_cur][c_cur] = result_cur;
            }
        }
        return result;
    }
}
matrix operator*(const matrix& left, int right)
{
    matrix result(left.x_length, left.y_length, 0);
    for (size_t i = 0; i < left.x_length; i++)
    {
        for (size_t j = 0; j < left.y_length; j++)
        {
            result[i][j] = left[i][j] * right;
        }
    }
    return result;
}
matrix matrix::operator<<(int right)
{
    if(right < 0) return (*this);
    matrix result(x_length, y_length, 0);
    for (size_t i = 0; i < x_length; i++)
    {
        for (size_t j = 0; j < y_length; j++)
        {
            result[i][j] = (*this)[i][j] << right;
        }
    }
    return result;
}
matrix operator>>(const matrix& left, int right)
{
    matrix result(left.x_length, left.y_length, 0);
    for (size_t i = 0; i < left.x_length; i++)
    {
        for (size_t j = 0; j < left.y_length; j++)
        {
            result[i][j] = left[i][j] >> right;
        }
    }
    return result;
}

matrix matrix::operator+(const matrix& right)
{
    matrix result(this->x_length, this->y_length, 0);
    for (uint i = 0; i < this->x_length; i++)
    {
        for (uint j = 0; j < this->y_length; j++)
        {result[i][j] = (*this)[i][j] + right[i][j];}
    }
    return result;
}
matrix& matrix::operator=(const matrix& right)
{
    for (uint i = 0; i < this->x_length; i++)
    {
        for (uint j = 0; j < this->y_length; j++)
        {(*this)[i][j] = right[i][j];}
    }
    return (*this);
}
bool matrix::operator>>(matrix& right)
{
    if(this->data == NULL ||\
       !(this->x_length == right.x_length && this->y_length == right.y_length)
      )
        return false;
    else
    {
        if(right.data != NULL) right.delete_data();
        right.data = this->data;
        this->data = NULL;
        return true;
    }
}
bool matrix::operator<<(matrix& right)
{
    if(right.data == NULL)
        return false;
    else
    {
        if(this->data != NULL)  this->delete_data();
        this->data = right.data;
        this->x_length = right.x_length;
        this->y_length = right.y_length;
        right.data = NULL;
        return true;
    }
}


bool matrix::Set_r(int row, int value)
{
    if(row < 0) return false;
    else
    {
        for (int i = 0; i < y_length; i++)
        {(*this)[row][i] = value;}
        return true;
    }
}
bool matrix::Set_c(int column, int value)
{
    if(column < 0) return false;
    else
    {
        for (int i = 0; i < y_length; i++)
        {(*this)[i][column] = value;}
        return true;
    }
}
int matrix::Sum_r(int row)
{
    int result = 0;
    for (uint i = 0; i < y_length; i++)
    {
        result += (*this)[row][i];
    }
    return result;
}
int matrix::Sum_c(int column)
{
    int result = 0;
    for (uint i = 0; i < x_length; i++)
    {
        result += (*this)[i][column];
    }
    return result;
}
matrix::matrix(int r_length, int c_length, int value):array2d<int>(r_length, c_length, value)
{
    x_length = r_length;
    y_length = c_length;
}

matrix::matrix(int r_length, int c_length):array2d<int>(r_length, c_length)
{
    x_length = r_length;
    y_length = c_length;
}

matrix::matrix():array2d<int>()
{
    
}
matrix::~matrix()
{
    
}