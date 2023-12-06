/* RAII全称是“Resource Acquisition is Initialization”，直译过来是“资源获取即初始化”.
 * 在构造函数中申请分配资源，在析构函数中释放资源。
 * 因为C++的语言机制保证了，当一个对象创建的时候，自动调用构造函数，当对象超出作用域的时候会自动调用析构函数。
 * 所以，在RAII的指导下，我们应该使用类来管理资源，将资源和对象的生命周期绑定
 * RAII的核心思想是将资源或者状态与对象的生命周期绑定，
 * 通过C++的语言机制，实现资源和状态的安全管理,智能指针是RAII最好的例子
 * */
#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

// 信号量
class sem
{
public:
    sem()
    {
        // 初始化一个未命名的信号量
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }
    ~sem()
    {
        sem_destroy(&m_sem);            // 销毁信号量
    }
    bool wait()
    {
        // 以原子操作方式将信号量减一,信号量为0时,sem_wait阻塞
        return sem_wait(&m_sem) == 0;
    }
    bool post()
    {
        // 以原子操作方式将信号量加一,信号量大于0时,唤醒调用sem_post的线程
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

// 互斥锁
class locker
{
public:
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)    // 初始化互斥锁
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);                // 销毁互斥锁
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;       // 以原子操作方式给互斥锁加锁
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;     // 以原子操作方式给互斥锁解锁
    }

    pthread_mutex_t *get()
    {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

// 条件变量
// 供了一种线程间的通知机制,当某个共享数据达到某个值时,唤醒等待这个共享数据的线程.
class cond
{
public:
    cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)  // 初始化条件变量
        {
            //pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    ~cond()
    {
        pthread_cond_destroy(&m_cond);              // 销毁条件变量
    }
    
    // 等待目标条件变量.该函数调用时需要传入 mutex 参数(加锁的互斥锁) ,
    // 函数执行时,先把调用线程放入条件变量的请求队列,
    // 然后将互斥锁mutex解锁,当函数成功返回为0时,互斥锁会再次被锁上. 
    // 也就是说函数内部会有一次解锁和加锁操作.
    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_wait(&m_cond, m_mutex);
        // 将线程放在条件变量的请求队列后，内部解锁
        // 线程等待被pthread_cond_broadcast信号唤醒或者pthread_cond_signal信号唤醒，唤醒后去竞争锁
        // 若竞争到互斥锁，内部再次加锁
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t)
    {
        int ret = 0;
        //pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        //pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;            // 以广播的方式唤醒所有等待目标条件变量的线程
    }

private:
    //static pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};
#endif
