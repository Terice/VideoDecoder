#ifndef MATRIX_H__
#define MATRIX_H__


#include "array2d.h"
#include <iostream>
class matrix :public array2d<int>
{
private:
    matrix& Foreach(void(*f)());
public:

    matrix& new_array(int length, int x, int y);

    matrix operator<<(int right);
    bool operator>>(matrix& right);
    bool operator<<(matrix& right);
    matrix& operator=(const matrix&);
    matrix operator+(const matrix&);
    matrix& operator+=(const matrix&);
    matrix& operator+=(int);
    matrix& operator*=(int);
    matrix& operator/=(int);
    friend matrix operator*(const matrix& left, const matrix& right);
    friend matrix operator*(const matrix& left, int right);
    friend matrix operator>>(const matrix& left,int right);
    friend matrix operator+(const matrix& left, const int right);
    friend bool operator<(array2d<int>& dst, matrix& ma);
    friend bool operator>(array2d<int>& dst, matrix& ma);
    friend std::ostream&  operator<<(std::ostream& out ,const matrix& ma);
    bool Set_r(int row, int value);
    int Sum_r(int row);
    bool Set_c(int column, int value);
    int Sum_c(int column);

    matrix(int r_length, int c_length, int value);
    matrix(int r_length, int c_length);
    matrix(array2d<int>* res);
    matrix();
    ~matrix();
};
#endif