# A C++ High Performance Web Server


## Introduction  
参考linyacool的[Web Server](https://github.com/linyacool/WebServer )和 chenshuo的[muduo](https://github.com/chenshuo/muduo)。

前期主要是学习阅读源码，对比muduo的功能和实现，基本上将所有的类都注释过了。

后期待改进计划: 

1. 当前定时器不能精确的处理定时任务，后面改用timefd来实现定时器；
2. 将socket fd和其操作封装为RAII类；
3. 目前用std::string做读写缓冲，性能有限，后面参考muduo实现自己的读写缓冲；
4. 目前的Server类比较混乱，后面进行整理拆分，拆成TcpServer，TcpConnect和Accpetor，支持主动关闭和被动关闭；
5. 支持更多的HTTP请求方法和返回状态码(当前只支持400，404)；
6. 支持HTTPS。

本项目为C++11编写的Web服务器，解析了get请求，可处理静态资源，支持HTTP长连接。

## Envoirment  
* OS: Ubuntu 14.04
* Complier: g++ 4.8

## Build

	./build.sh

## Usage

	./WebServer [-t thread_numbers] [-p port] [-l log_file_path(should begin with '/')]

## Technical points
* 使用Epoll边沿触发的IO多路复用技术，非阻塞IO，使用Reactor模式
* 使用多线程充分利用多核CPU，并使用线程池避免线程频繁创建销毁的开销
* 使用基于小根堆的定时器关闭超时请求
* 主线程只负责accept请求，并以Round Robin的方式分发给其它IO线程(兼计算线程)，锁的争用只会出现在主线程和某一特定线程中
* 使用eventfd实现了线程的异步唤醒
* 为减少内存泄漏的可能，使用智能指针等RAII机制
* 使用状态机解析了HTTP请求


