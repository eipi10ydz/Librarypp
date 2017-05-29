#ifndef _THREAD_POOL_
#define _THREAD_POOL_

#include <thread>
#include <future>
#include <mutex>
#include <condition_variable>

#include <functional>
#include <vector>
#include <queue>
#include <cstdint>

namespace eipi10
{
    class thread_pool
    {
    public:
        thread_pool(uint32_t = 0);
        virtual ~thread_pool();
        
        void start();
        void stop();
        void modify_thread_cnt(int64_t);
        uint32_t get_thread_cnt() { return thread_cnt; }

        template<typename Func, typename ...Args>
        std::future<typename std::result_of<Func(Args...)>::type> push(Func &&func, Args && ...args);

    private:
        void add_thread(uint32_t, uint32_t);

        struct thread_with_state
        {
            std::thread thread;
            bool is_running;
            thread_with_state() = default;
            thread_with_state(std::thread &&thread, bool is_running) : thread(std::move(thread)), is_running(is_running) { }
        };

        std::vector<thread_with_state> workers;
        std::queue<std::function<void()>> tasks;

        bool is_running;
        std::mutex queue_lock;
        std::condition_variable cv;
        uint32_t thread_cnt;
    };

    template<typename Func, typename ...Args>
    std::future<typename std::result_of<Func(Args...)>::type> thread_pool::push(Func &&func, Args &&...args)
    {
        using ret_type = typename std::result_of<Func(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<ret_type()>>(std::forward<Func>(func), std::forward<Args>(args)...);

        std::future<ret_type> res = task->get_future();
        {
            std::lock_guard<std::mutex> lock(queue_lock);
            if (!is_running)
            {
                throw std::runtime_error("thread pool is not running...");
            }
            tasks.emplace([task]{ (*task)(); });
        }
        cv.notify_one();

        return res;
    }
}

#endif
