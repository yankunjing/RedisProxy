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
    // �¼��̱߳��
    bool m_event_stop;

    // �¼����鼯��
    std::vector<std::thread> m_event_threads;

    // �¼��̵߳��ź���
    std::mutex m_event_mutex;

    // �¼��̵߳���������
    std::condition_variable m_event_cvar;
};

typedef Container::singleton_default<CMsgWorkerManager> MsgWorkerManager;

#endif // !_MSG_WORKER_MANAGER_H
