#include "epoll.h"

Epoller::Epoller(int maxEvent) : epollerFd_(epoll_create(512)), events_(maxEvent)
{
    assert(epollerFd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller()
{
    close(epollerFd_);
}

//将描述符fd加入epoll监控
bool Epoller::addFd(int fd, uint32_t events)
{
    if (fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollerFd_, EPOLL_CTL_ADD, fd, &ev);
}

//修改描述符fd对应的事件
bool Epoller::modFd(int fd,uint32_t events)
{
    if (fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollerFd_, EPOLL_CTL_MOD, fd, &ev); 
}

//将描述符fd移除epoll的监控
bool Epoller::delFd(int fd)
{
    if (fd < 0) return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(epollerFd_, EPOLL_CTL_DEL, fd, &ev); 
}

//用于返回监控的结果，成功时返回就绪的文件描述符的个数
int Epoller::wait(int timeoutMs)
{
    epoll_wait(epollerFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

//获取fd的函数
int Epoller::getEventFd(size_t i) const
{
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
}

//获取events的函数
uint32_t Epoller::getEvents(size_t i) const
{
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}