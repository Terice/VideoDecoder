#ifndef ARRAT2D_H__
#define ARRAT2D_H__

#include <iostream>
template <class T>
class array2d
{
private:
public:
    T* data;
    int x_length;
    int y_length;
    T& get_value_xy(int x,  int y);
    T& get_value_i(int i)const;
    bool set_value( int x,  int y, T value);
    bool set_value( int i, T value);
    bool is_avaiable(int x,  int y);
    bool print_element(int x, int y);
    bool read_from(T* right);


    int get_xlength() const{return x_length;}
    int get_ylength() const{return y_length;}
    //the first[] is row, second[] is column
    T* operator[](int row) const;
    bool operator=(array2d<T>& right);
    bool delete_data();
    bool init_value(int x, int y, const T& initvalue);
    array2d(int xinitlength,  int yinitlength, const T& initvalue);
    array2d(int xinitlength,  int yinitlength);
    array2d(const array2d<T>& a);
    array2d();
    ~array2d();
};

template <class T>
bool array2d<T>::init_value(int x, int y, const T& initvalue)
{
    if(data != NULL) return false;
    else
    {
        data = new T[x*y];
        for(int i = 0; i < x * y; i++)
            data[i] = initvalue;
    }
    return true;
}

template <class T>
bool array2d<T>::read_from(T* right)
{
    if(right == NULL) return false;
    else 
        for (int i = 0; i < x_length * y_length; i++)
            data[i] = right[i];
    return true;
}
template <class T>
bool array2d<T>::operator=(array2d<T>& right)
{
    if(right == NULL) return false;
    else    
        for (int i = 0; i < x_length; i++)
            for (int j = 0; j < y_length; j++)
                (*this)[i][j] = right[i][j];
    return true;
}
template <class T>
T* array2d<T>::operator[](int x) const
{
    if(x >= x_length || x < 0) return NULL;
    else return data + (x * y_length);
}
template <class T>
bool array2d<T>::is_avaiable(int x,  int y)
{
    if( x < 0 || y < 0 || x >= x_length || y >= y_length) return false;
    else return true;
}
template <class T>
bool array2d<T>::print_element(int x, int y)
{
    if(x >= x_length || y >= y_length || x < 0 || y < 0) return false;
    else std::cout << data[x * y_length + y] << std::endl;
}
template <class T>
T& array2d<T>::get_value_i(int i)const
{
    return data[i];
}
template <class T>
T& array2d<T>::get_value_xy( int x,  int y)
{
    return data[x * y_length + y];
}
template <class T>
bool array2d<T>::set_value( int x,  int y, T value)
{
    if(x >= x_length || y >= y_length || x < 0 || y < 0) return false;
    else {data[x * y_length + y] = value; return true;}
}
template <class T>
bool array2d<T>::set_value( int i, T value)
{
    if(i >= x_length * y_length || i < 0) return false;
    else {data[i] = value; return true;}
}
template <class T>
bool array2d<T>::delete_data()
{if(data != NULL) {delete[] data; data = NULL;return true;} return true;}
template <class T>
//get an empty array(no data area)
array2d<T>::array2d(int xinitlength,  int yinitlength)
{
    x_length = xinitlength;
    y_length = yinitlength;
    data = NULL;
}
template <class T>
//get an data area and init it value
array2d<T>::array2d(int xinitlength,  int yinitlength, const T& initvalue)
{
    data = new T[xinitlength * yinitlength];
    x_length = xinitlength;
    y_length = yinitlength;
    for(int i = 0; i < x_length * y_length; i++) {data[i] = initvalue;}
}
template <class T>
array2d<T>::array2d()
{
    x_length = 0;
    y_length = 0;
    data = NULL;
}

template <class T>
array2d<T>::array2d(const array2d<T>& a)
{
    x_length = a.get_xlength();
    y_length = a.get_ylength();
    data = new T[x_length * y_length];
    for(size_t i = 0; i < x_length * y_length; i++)
    {data[i] = a.get_value_i(i);}
}


template <class T>
array2d<T>::~array2d()
{
    if(data != NULL)
    {
        delete[] data;
        data = NULL; 
    }
}


#endif