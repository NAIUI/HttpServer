#include "webserver.h"

WebServer::WebServer(
    int port,int trigMode,int timeoutMS,bool optLinger,int threadNum):
    port_(port),
    openLinger_(optLinger),
    timeoutMS_(timeoutMS),
    isClose_(false),
    timer_(new TimerManager()),
    threadpool_(new ThreadPool(threadNum)),
    epoller_(new Epoller())
{
    //获取当前工作目录的绝对路径
    srcDir_ = getcwd(nullptr,256);
    assert(srcDir_);
    //拼接字符串
    strncat(srcDir_,"/resources/",16);
    HTTPconnection::userCount = 0;
    HTTPconnection::srcDir = srcDir_;

    initEventMode_(trigMode);
    if(!initSocket_()) isClose_=true;

}

WebServer::~WebServer()
{
    close(listenFd_);
    isClose_=true;
    free(srcDir_);
}

// 初始化监听socket与连接socket的属性
void WebServer::initEventMode_(int trigMode) {
    listenEvent_ = EPOLLRDHUP;                      // 检测到对端关闭时，及时进行清理和处理。仅在使用 ET 模式时才有效
    /* EPOLLONESHOT
     * 确保每个事件只被一个处理线程处理，避免多个线程同时处理同一个事件。
     * epoll 事件驱动机制中的一个标志。
     * 处理完当前事件后，将文件描述符从 epoll 集合中摘除
     * 不会再收到这个文件描述符相关的事件，
     * 直到该文件描述符再次通过 epoll_ctl 重新加入。
     * */
    connectionEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connectionEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connectionEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connectionEvent_ |= EPOLLET;
        break;
    }
    HTTPconnection::isET = (connectionEvent_ & EPOLLET); // 检查（与操作）连接事件是否包含了 EPOLLET 标志
}

void WebServer::Start()
{
    int timeMS=-1;//epoll wait timeout==-1就是无事件一直阻塞
    if(!isClose_) 
    {
        std::cout<<"============================";
        std::cout<<"Server Start!";
        std::cout<<"============================";
        std::cout<<std::endl;
    }
    while(!isClose_)
    {
        if(timeoutMS_>0)
        {
            timeMS=timer_->getNextHandle();         // 更新最快过期时间
        }
        int eventCnt=epoller_->wait(timeMS);        // 阻塞
        for(int i=0;i<eventCnt;++i)
        {
            int fd=epoller_->getEventFd(i);
            uint32_t events=epoller_->getEvents(i);

            if(fd==listenFd_)
            {
                handleListen_();
                // std::cout << "handleListen_ " << std::endl;
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users_.count(fd) > 0);
                closeConn_(&users_[fd]);
            }
            else if(events & EPOLLIN) {
                assert(users_.count(fd) > 0);
                handleRead_(&users_[fd]);
                // std::cout << "handleRead_ " << std::endl;
            }
            else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                handleWrite_(&users_[fd]);
                // std::cout << "handleWrite_ " << std::endl;
            } 
            else {
                std::cout<<"Unexpected event"<<std::endl;
            }
        }
    }
}

void WebServer::sendError_(int fd, const char* info)
{
    assert(fd>0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret<0)
    {
        std::cout<<"send error to client"<<fd<<" error!"<<std::endl;
    }
    close(fd);
}

void WebServer::closeConn_(HTTPconnection* client)
{
    assert(client);
    epoller_->delFd(client->getFd());
    client->closeHTTPConn();
}

void WebServer::addClientConnection(int fd, sockaddr_in addr)
{
    assert(fd>0);
    // HTTPconnection *httpConn = new HTTPconnection();
    // httpConn->iinitHTTPConn(fd, addr);
    // users_[fd] = httpConn
    users_[fd].initHTTPConn(fd, addr);      // 直接构造HTTPConnection对象，调用初始化函数
    if(timeoutMS_>0)
    {
        // std::bind  函数对象适配器，用于将成员函数或普通函数与参数绑定，形成一个可调用对象（函数对象）
        timer_->addTimer(fd,timeoutMS_,std::bind(&WebServer::closeConn_,this,&users_[fd]));
    }
    epoller_->addFd(fd,EPOLLIN | connectionEvent_);
    setFdNonblock(fd);
}

void WebServer::handleListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do {
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0) { return;}
        else if(HTTPconnection::userCount >= MAX_FD) {
            sendError_(fd, "Server busy!");
            return;
        }
        addClientConnection(fd, addr);          // 添加该HTTP连接
        // std::cout << "listenEvent_ & EPOLLET " << (listenEvent_ & EPOLLET) << std::endl;
    } while(listenEvent_ & EPOLLET);            // 在边缘触发模式下循环处理多个连接。
}

void WebServer::handleRead_(HTTPconnection* client) {
    assert(client);
    extentTime_(client);       
    threadpool_->submit(std::bind(&WebServer::onRead_, this, client));
}

void WebServer::handleWrite_(HTTPconnection* client)
{
    assert(client);
    extentTime_(client);
    threadpool_->submit(std::bind(&WebServer::onWrite_, this, client));
}

// 更新过期时间
void WebServer::extentTime_(HTTPconnection* client)
{
    assert(client);
    if(timeoutMS_>0)
    {
        timer_->update(client->getFd(),timeoutMS_);
    }
}

void WebServer::onRead_(HTTPconnection* client) 
{
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->readBuffer(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        /*在错误处理的上下文中，EAGAIN 表示当前时间点操作无法立即完成，但在稍后的某个时间点可能会成功*/
        closeConn_(client);
        return;
    }
    onProcess_(client);
}

void WebServer::onProcess_(HTTPconnection* client) 
{
    if(client->handleHTTPConn()) {
        epoller_->modFd(client->getFd(), connectionEvent_ | EPOLLOUT);
    } 
    else {
        epoller_->modFd(client->getFd(), connectionEvent_ | EPOLLIN);
    }
}

void WebServer::onWrite_(HTTPconnection* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->writeBuffer(&writeErrno);
    if(client->writeBytes() == 0) {
        /* 传输完成 */
        if(client->isKeepAlive()) {
            onProcess_(client);
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller_->modFd(client->getFd(), connectionEvent_ | EPOLLOUT);
            return;
        }
    }
    closeConn_(client);
}

// 初始化监听socket ， 非阻塞模式，加入读事件监听，启动端口复用
bool WebServer::initSocket_() {
    int ret;
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) {
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);
    struct linger optLinger = { 0 };
    if(openLinger_) {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0) {
        return false;
    }

    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret == -1) {
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if(ret < 0) {
        close(listenFd_);
        return false;
    }
    ret = epoller_->addFd(listenFd_,  listenEvent_ | EPOLLIN);      // 读事件加入
    if(ret == 0) {
        close(listenFd_);
        return false;
    }
    setFdNonblock(listenFd_);
    return true;
}

int WebServer::setFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}