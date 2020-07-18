# NALreader  
a simple and Incomplete reader for h.264  

**for learning**  


## 运行对象  

解析器parser用来读取和存储一些句法元素(主要是SPS PPS)  
cabac的运行也是在parser中完成的  
从文件中读取字节的处理由parser中的bitsbuf对象完成  

picture对象存储每一帧的宏块的数据  
picture对象由Decoder对象管理  

Debug对象用来管理各种信息的打印  
控制符在对应的lua脚本中  

## 未完成的点：  
- 只能用于熵编码的264，不能用于非熵编码的264  
- B宏块时间空间预测全部简化为了空间预测随周围宏块一起运动  
- 只有帧模式，只能用420，只解亮度  
- 解码速度很慢，大概1秒1帧（大哭/(ㄒoㄒ)/~~），  
- 没有做PCM宏块的处理  
- 其他很多--  