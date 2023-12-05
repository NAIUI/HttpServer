# WebServer
用C++实现的高性能WEB服务器，经过webbenchh压力测试可以实现上万的QPS

## 功能
* 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
* 利用正则与状态机解析HTTP请求报文，实现处理静态资源的请求；
* 利用标准库容器封装char，实现自动增长的缓冲区；
* 基于小根堆实现的定时器，关闭超时的非活动连接；
* 利用单例模式与阻塞队列实现异步的日志系统，记录服务器运行状态；
* 利用RAII机制实现了数据库连接池，减少数据库连接建立与关闭的开销，同时实现了用户注册登录功能。

* 增加logsys,threadpool测试单元(todo: timer, sqlconnpool, httprequest, httpresponse) 

## 环境要求
* Linux
* C++14
* MySql

## 目录树
```
.
├── code           源代码
│   ├── buffer
│   ├── config
│   ├── http
│   ├── log
│   ├── timer
│   ├── pool
│   ├── server
│   └── main.cpp
├── test           单元测试
│   ├── Makefile
│   └── test.cpp
├── resources      静态资源
│   ├── index.html
│   ├── image
│   ├── video
│   ├── js
│   └── css
├── bin            可执行文件
│   └── server
├── log            日志文件
├── webbench-1.5   压力测试
├── build          
│   └── Makefile
├── Makefile
├── LICENSE
└── readme.md
```


## 项目启动
需要先配置好对应的数据库
```bash
// 建立yourdb库
create database yourdb;

// 创建user表
USE yourdb;
CREATE TABLE user(
    username char(50) NULL,
    password char(50) NULL
)ENGINE=InnoDB;

// 添加数据
INSERT INTO user(username, password) VALUES('name', 'password');
```

```bash
make
./bin/server
```

## 单元测试
```bash
cd test
make
./test
```

## 压力测试
![image-webbench](https://github.com/markparticle/WebServer/blob/master/readme.assest/%E5%8E%8B%E5%8A%9B%E6%B5%8B%E8%AF%95.png)
```bash
./webbench-1.5/webbench -c 100 -t 10 http://ip:port/
./webbench-1.5/webbench -c 1000 -t 10 http://ip:port/
./webbench-1.5/webbench -c 5000 -t 10 http://ip:port/
./webbench-1.5/webbench -c 10000 -t 10 http://ip:port/
```
* 测试环境: Ubuntu:19.10 cpu:i5-8400 内存:8G 
* QPS 10000+

## TODO
* config配置
* 完善单元测试
* 实现循环缓冲区

## 致谢
Linux高性能服务器编程，游双著.

[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)

# WebServer

用C++实现的高性能WEB服务器，经过webbenchh压力测试可以实现上万的QPS

项目地址：https://github.com/Aged-cat/WebServer

## 功能

- 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
- 利用正则与状态机解析HTTP请求报文，实现处理静态资源的请求；
- 利用标准库容器封装char，实现自动增长的缓冲区；
- 基于堆结构实现的定时器，关闭超时的非活动连接；
- 改进了线程池的实现，QPS提升了45%+；

## 项目详解

[WebServer项目——buffer详解](https://www.agedcat.com/post/66ac85ff.html)

[WebServer项目——epoller详解](https://www.agedcat.com/post/bcf82ea4.html)

[WebServer项目——timer详解](https://www.agedcat.com/post/f410f66e.html)

[WebServer项目——threadpool详解](https://www.agedcat.com/post/678e35eb.html)

[WebServer项目——HTTPconnection详解](https://www.agedcat.com/post/8b7b6922.html)

[WebServer项目——HTTPrequest详解](https://www.agedcat.com/post/8b528cc7.html)

[WebServer项目——HTTPresponse详解](https://www.agedcat.com/post/5b7b173e.html)

[WebServer项目——webserver详解](https://www.agedcat.com/post/1308746a.html)

## 环境要求

- Linux
- C++11

## 项目启动

```
mkdir bin
make
./bin/myserver
```

## 压力测试

```
./webbench-1.5/webbench -c 100 -t 10 http://ip:port/
./webbench-1.5/webbench -c 1000 -t 10 http://ip:port/
./webbench-1.5/webbench -c 5000 -t 10 http://ip:port/
./webbench-1.5/webbench -c 10000 -t 10 http://ip:port/
```

- 测试环境: Ubuntu:20 cpu:i7-4790 内存:16G

## 性能表现

与[markparticle](https://github.com/markparticle/WebServer/)的C++服务器做一个比较(表格中的为QPS的值)：

|      |  10   |  100  | 1000  | 10000 |
| :--: | :---: | :---: | :---: | :---: |
| old  | 8929  | 9126  | 9209  |  155  |
| new  | 11478 | 13578 | 13375 |  106  |

性能提升了45%

## 致谢

@[markparticle](https://github.com/markparticle/WebServer/)   
@[agedcat](https://github.com/agedcat/WebServer)



线程同步机制包装类
===============
多线程同步，确保任一时刻只能有一个线程能进入关键代码段.
> * 信号量
> * 互斥锁
> * 条件变量





同步/异步日志系统
===============
同步/异步日志系统主要涉及了两个模块，一个是日志模块，一个是阻塞队列模块,其中加入阻塞队列模块主要是解决异步写入日志做准备.
> * 自定义阻塞队列
> * 单例模式创建日志
> * 同步日志
> * 异步日志
> * 实现按天、超行分类
1. 本项目中，使用单例模式创建日志系统，对服务器运行状态、错误信息和访问数据进行记录，该系统可以实现按天分类，超行分类功能，可以根据实际情况分别使用同步和异步写入两种方式。
2. 其中异步写入方式，将生产者-消费者模型封装为阻塞队列，创建一个写线程，工作线程将要写的内容push进队列，写线程从队列中取出内容，写入日志文件。
3. 日志系统大致可以分成两部分，其一是单例模式与阻塞队列的定义，其二是日志类的定义与使用。