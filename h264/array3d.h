#ifndef ARRAT3D_H__
#define ARRAT3D_H__


template <class T>
class array3d
{
private:
    T* data;
    int x_length;
    int y_length;
    int z_length;
public:
    T get_value( int x,  int y,  int z);
    bool set_value( int x,  int y,  int z, T value);
    array3d( int xinitlength,  int yinitlength,  int zinitlength);
    bool is_avaiable( int x,  int y,  int z);
    ~array3d();
};

template <class T>
bool array3d<T>::is_avaiable(int x,  int y,  int z)
{
    if(x >= x_length || y >= y_length || z * z_length || x < 0 || y < 0 || z < 0) return false;
    else return true;
}
template <class T>
T array3d<T>::get_value( int x,  int y,  int z)
{
    if(x >= x_length || y >= y_length || z * z_length) return (T)0;
    else return data[x * y_length + y * z_length + z];
}
template <class T>
bool array3d<T>::set_value( int x,  int y,  int z, T value)
{
    if(x >= x_length || y >= y_length || z >= z_length) return false;
    else {data[x * y_length + y * z_length + z] = value; return true;}
}
template <class T>
array3d<T>::array3d( int xinitlength,  int yinitlength,  int zinitlength)
{
    data = new T[xinitlength * yinitlength * zinitlength];
    x_length = xinitlength;
    y_length = yinitlength;
    z_length = zinitlength;
}
template <class T>
array3d<T>::~array3d()
{
    delete[] data;
}


#endif