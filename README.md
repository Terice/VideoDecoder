# NALreader
a simple and Incomplete reader for h.264

**for learning**


# 运行流程

解析器parser用来读取和存储一些句法元素(主要是SPS PPS)
cabac的运行也是在parser中完成的
从文件中读取字节的处理由parser中的bitsbuf对象完成

picture对象存储每一帧的宏块的数据
picture对象由Decoder对象管理

Debug对象用来管理各种信息的打印
控制符在对应的lua脚本中

主要未完成的点：
1、只能用于熵编码的264，不能用于非熵编码的264
2、没有排序、参考列表没有写
3、没有解B帧和B宏块
4、只有帧模式，只能用420，只解亮度
5、解码速度很慢，大概3秒10帧，
6、没有做PCM宏块的处理
7、其他很多--