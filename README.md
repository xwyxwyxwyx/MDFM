# MDFM(Mobile Device File Monitor)

## 简介

MDFM是一个基于Minifilter的透明加解密系统，其主要监控的目标是可移动设备上的文件。MDFM旨在让微小企业或团队在使用的 “共享的” 可移动设备（U盘、机械硬盘、SSD等）时，能够使不同进程对设备上的文件进行分级权限访问，防止信息泄露。

MDFM的控制端实现：设置准入进程、设置保护设备、设置不同客户端（不同主机）对文件的操作权限。

MDFM的客户端实现：当系统内的主机访问收保护的文件时，如果是准入进程，那么为其进行文件的透明加解密，如果是普通进程，不提供解密。同时，为了防止普通进程从文件缓冲区读取到已经被解密的明文。当普通进程想访问一个正在被加密的文件时，应该直接拒绝。



By the way，这都是作者脑子里面的yy。本项目是作者在学习驱动开发后实践的第一个项目，具有学习与实践性质，项目中使用的技术思路可能不是最好的，甚至项目本身会存在很多缺陷。因此本项目只具参考价值，与学习驱动开发的诸君共勉。



## 开发进度

**2023-2-6	项目立项**

**2023-2-7	完成了进程识别的相关例程**

**2023-2-9	实现了基于SwapBuffer的异或加密**

**2023-2-11  完善了内核与应用层通信的接口**

**2023-2-14  实现了透明读写**

**2023-2-18  完善了透明加解密机制，暂时关闭了异或加密算法**



## 参考

https://github.com/microsoft/Windows-driver-samples/tree/master/filesys/miniFilter/swapBuffers

https://github.com/minglinchen/WinKernelDev/tree/master/crypt_file

https://github.com/hkx3upper/FOKS-TROT

https://github.com/hkx3upper/Minifilter

https://github.com/kernweak/minicrypt

《Windows内核编程》——谭文



## PS：过滤的原理

create:

- check 过滤路径
- check 进程
  - 禁止非授权进程访问保密区
  - 对授权进程继续放行
    - 创建/打开文件流上下文
    - check标识
      - 无文件标识，添加之，填充文件流上下文
      - 有文件标识，填充文件流上下文

​	

read/write：

- check 进程
- check 过滤路径
- check 文件标识
  - 完成加解密



cleanup：

- check进程
- check过滤路径
  - 清除缓存
  - 更新并写回Flag



## 日志

2023-2-17：在做透明读写时，文件的读需要设置偏移量ByteOffset加上文件头的长度。但是文件的写并不需要设置ByteOffset，想要写文件的位置是正确的，必须要完成“查询文件大小”、“设置文件大小”对应的两个回调函数。

2023-2-18: 更新文件标识的工作不能放在IRP_MJ_CLOSE的回调上，只能放在IRP_MJ_CLEANUP。放在close中莫名其妙无法完成Flag的写入，但是系统也不会有任何错误产生。







