# 内存池 mmpool 
    mmpool内存池和nginx中的内存池基本一致  
    mmpool内存池详解文档：https://note.youdao.com/s/HhW6G8A2  
    mmpool内存池中，**分小块内存和大块内存，小块内存只能在内存池销毁后才能释放内存，大块内存分配后，可以随时释放**。  
    内存池代码：mmpool.c和mmpool.h  
    测试内存池代码：test_mmpool_main.c  
## 1、内存池的相关结构和开始创建内存池时需分配的结构空间
![image](https://user-images.githubusercontent.com/35031390/204705153-523f2617-70ab-48d6-a9ce-0704242a2ed7.png)
## 2、内存池怎么申请大块内存与小块内存的
### 2.1、流程图方式解释用户向内存池申请内存过程
![image](https://user-images.githubusercontent.com/35031390/204705351-4241e36d-cecf-4c96-aa1e-510cabf8060d.png)
### 2.2 内存池中大小块内存怎么串联起来的
![image](https://user-images.githubusercontent.com/35031390/204705642-1d26fb13-9169-4584-a984-35d5201c1d24.png)
