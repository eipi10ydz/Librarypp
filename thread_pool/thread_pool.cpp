#include "thread_pool.h"
#include <cassert>

namespace eipi10
{
    thread_pool::thread_pool(uint32_t thread_cnt) : is_running(false), thread_cnt(thread_cnt)
    {
        if (thread_cnt == 0)
            thread_cnt = std::thread::hardware_concurrency();
    }

    void thread_pool::start()
    {
        is_running = true;
        add_thread(0, thread_cnt);
        /*
        for (uint32_t i = 0; i < thread_cnt; ++i)
        {
            workers.emplace_back(std::thread([this, i](){
                for ( ; ; )
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_lock);
                        cv.wait(lock, [this, i](){
                            return !workers[i].is_running || !this->is_running 
                            || (this->is_running && workers[i].is_running && !tasks.empty());
                        });

                        task = tasks.front();
                        tasks.pop();
                    }
                    task();
                }
            }), true);
        }
        */
    }

    void thread_pool::modify_thread_cnt(int64_t modify_cnt)
    {
        uint32_t tmp_cnt;
        if (modify_cnt < 0)
        {
            int64_t tmp = static_cast<int64_t>(thread_cnt) + modify_cnt;
            assert(tmp > 0);
            tmp_cnt = tmp;
            {
                std::lock_guard<std::mutex> lock(queue_lock);
                for (uint32_t i = tmp_cnt; i < thread_cnt; ++i)
                {
                    workers[i].is_running = false;
                }
            }
            cv.notify_all();

            for (uint32_t i = tmp_cnt; i < thread_cnt; ++i)
                if (workers[i].thread.joinable())
                    workers[i].thread.join();

            workers.resize(tmp_cnt);
        }
        else
        {
            // maybe overflow...
            tmp_cnt = thread_cnt + modify_cnt;
            assert(thread_cnt < tmp_cnt);

            add_thread(thread_cnt, tmp_cnt);
            /*
            for (uint32_t i = thread_cnt; i < tmp_cnt; ++i)
            {
                workers.emplace_back(std::thread([this, i](){
                    for ( ; ; )
                    {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queue_lock);
                            cv.wait(lock, [this, i](){
                                return !workers[i].is_running || !this->is_running 
                                || (this->is_running && workers[i].is_running && !tasks.empty());
                            });

                            task = tasks.front();
                            tasks.pop();
                        }
                        task();
                    }
                }), true);
            }
            */
        }
        thread_cnt = tmp_cnt;
    }

    void thread_pool::add_thread(uint32_t beg, uint32_t end)
    {
        for (uint32_t i = beg; i < end; ++i)
        {
            workers.emplace_back(std::thread([this, i](){
                for ( ; ; )
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_lock);
                        cv.wait(lock, [this, i](){
                            return !workers[i].is_running || !this->is_running 
                            || (this->is_running && workers[i].is_running && !tasks.empty());
                        });

                        if ((!this->is_running && tasks.empty()) || !workers[i].is_running)
                            return;

                        task = tasks.front();
                        tasks.pop();
                    }
                    task();
                }
            }), true);
        }
    }

    void thread_pool::stop()
    {
        {
            std::lock_guard<std::mutex> lock(queue_lock);
            if (is_running)
                is_running = false;
        }
        cv.notify_all();

        for (auto &worker : workers)
            if (worker.thread.joinable())
                worker.thread.join();
    }

    thread_pool::~thread_pool()
    {
        stop();
    }
}
