# MDFM(Mobile Device File Monitor)

## 简介

MDFM是一个基于Minifilter的透明加解密系统，其主要监控的目标是可移动设备上的文件。MDFM旨在让微小企业或团队在使用的 “共享的” 可移动设备（U盘、机械硬盘、SSD等）时，能够对设备上的文件进行分级权限访问，防止信息泄露。

MDFM的控制端实现：设置准入进程、设置保护设备、设置不同客户端对文件的操作权限。

MDFM的客户端实现：当系统内的主机访问收保护的文件时，如果是准入进程，那么为其进行文件的透明加解密，如果是普通进程，那么直接拒绝其访问收保护的文件。



By the way，这都是作者脑子里面的yy。本项目是作者在学习驱动开发后实践的第一个项目，具有学习与实践性质，项目中使用的技术思路可能不是最好的，甚至项目本身会存在很多缺陷。因此本项目只具参考价值，与学习驱动开发的诸君共勉。



## 开发进度

**2023-2-6	项目立项**

**2023-2-7	完成了进程识别的相关例程**

**2023-2-8	实现了基于SwapBuffer的异或加密**



## 参考

https://github.com/microsoft/Windows-driver-samples/tree/master/filesys/miniFilter/swapBuffers

https://github.com/minglinchen/WinKernelDev/tree/master/crypt_file

https://github.com/hkx3upper/FOKS-TROT

https://github.com/hkx3upper/Minifilter

https://github.com/kernweak/minicrypt

《Windows内核编程》——谭文



