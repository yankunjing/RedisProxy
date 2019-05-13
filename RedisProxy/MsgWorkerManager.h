#ifndef _MSG_WORKER_MANAGER_H
#define _MSG_WORKER_MANAGER_H

#include <vector>             // std::vector
#include <thread>             // std::thread
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable, std::cv_status
#include <functional>         // std::function
#include "Singleton.h"

class CMsgWorkerManager
{
public:
    CMsgWorkerManager();
    ~CMsgWorkerManager();

public:
    bool Start();
    void Stop();

private:
    using Job = std::function<void()>;

    void event_thread_daemon();

private:
    // 事件线程标记
    bool m_event_stop;

    // 事件数组集合
    std::vector<std::thread> m_event_threads;

    // 事件线程的信号量
    std::mutex m_event_mutex;

    // 事件线程的条件变量
    std::condition_variable m_event_cvar;
};

typedef Container::singleton_default<CMsgWorkerManager> MsgWorkerManager;

#endif // !_MSG_WORKER_MANAGER_H
