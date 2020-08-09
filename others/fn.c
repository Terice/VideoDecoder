// 我在 debug 时用的函数

#include <stdio.h>

// 功能：在一个 二进制 文件中查找一个指定的 unsigned char 串
// 参数：文件指针，字符串指针，字符串长度
// 返回：返回-1表示到达文件尾，返回其他则是在文件中该字符串起始的位置，-2表示未知错误
// 操作：文件指针会被移到这个字符串的结尾的下一个字符的位置
// 算法：朴素算法
// 注释：移动文件指针到匹配字符串的下一个字符的位置是为了方便直接开始下一次的查找
long f_findUcharStr(FILE* fp, unsigned char* str, int length)
{
    long result;
    int ch;
    long pos_0;
    while(1) // 第一层循环
    {
        size_t i;
        pos_0 = ftell(fp);
        for(i = 0; i < length; i++) // 第二层循环
        {
            // 如果当前字节到达文件尾，那么返回-1
            if((ch = fgetc(fp)) == EOF) return -1;
            
            // 如果当前字符匹配那么继续验证下一个字符是否匹配
            if((unsigned char)ch == str[i]) continue;
            // 否则退出循环
            else break;
        }
        // printf("|");
        // 如果匹配长度等于字符串的长度，那么文件指针越过这个字符串 返回字符串第一个字符所在的位置
        if(i == length) {fseek(fp, (long)length, SEEK_CUR); return pos_0;}
        // 否则进到下一个字符并且继续验证
        else fseek(fp, pos_0+1, SEEK_SET);
    }
    return result = -2; // 返回-2说明出现了未知错误，正常的返回在while中
}


// 功能：查找264文件中的所有 SPS 起始序列 的位置
// 注释：直接seek到264文件的某个SPS开头以便于从该处直接开始解码
int main()
{
    FILE* fp;
    unsigned char sps[4] = {0x00,0x00,0x01,0x67}; //SPS的开头序列
    
    long result;
	int index;
    
    if((fp = fopen("../resource/fox.264","r")) == NULL) return -1;
    
    for(index = 1, result = 0; result != -1; index++)
    {
        result = f_findUcharStr(fp, sps, 4);
        printf("%-3d: %d\n",index, result);
    }
	return 0;
}
