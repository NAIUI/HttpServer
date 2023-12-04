#include "threadpool.h"

ThreadPool::ThreadPool(size_t threadNumber):m_stop(false)
{
    for(size_t i=0;i<threadNumber;++i)
    {
        m_thread.emplace_back(
            [this](){
                for(;;)
                {
                    std::function<void()>task;
                    {
                        std::unique_lock<std::mutex>lk(m_mutex);
                        m_cv.wait(lk,[this](){ return m_stop||!tasks.empty();});
                        if(m_stop&&tasks.empty()) return;
                        task=std::move(tasks.front());
                        tasks.pop();
                    }
                    task();
                }
            }
        );
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_stop = true;
    }
    m_cv.notify_all();
    for(auto& threads:m_thread)
    {
        threads.join();
    }
}

// ---------------------- Test---------------------------------

// std::random_device rd;
// std::mt19937 mt(rd());  
// std::uniform_int_distribution<int> dist(-1000, 1000);
// auto rnd = std::bind(dist, mt);

// void simulate_hard_computation() {
//   std::this_thread::sleep_for(std::chrono::milliseconds(2000 + rnd()));
// }

// void multiply(const int a, const int b) {
//   simulate_hard_computation();
//   const int res = a * b;
//   std::cout << a << " * " << b << " = " << res << std::endl;
// }


// void multiply_output(int & out, const int a, const int b) {
//   simulate_hard_computation();
//   out = a * b;
//   std::cout << a << " * " << b << " = " << out << std::endl;
// }


// int multiply_return(const int a, const int b) {
//   simulate_hard_computation();
//   const int res = a * b;
//   std::cout << a << " * " << b << " = " << res << std::endl;
//   return res;
// }


// int main(void)
// {
//     ThreadPool pool(4);

//     for(int i=0;i<8;i++)
//     {
//         pool.submit(multiply,i,i+1);
//     }

//     system("pause");
//     return 0;
// }