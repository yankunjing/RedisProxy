#include "MsgWorkerManager.h"
#include "MsgQueueManager.h"
#include <algorithm>

CMsgWorkerManager::CMsgWorkerManager() : m_event_stop(false)
{
}

CMsgWorkerManager::~CMsgWorkerManager()
{
}

bool CMsgWorkerManager::Start()
{
    if (m_event_threads.size()) {
        return true;
    }

    // 获取cpu核心个数
    unsigned int cpu_core_num = std::thread::hardware_concurrency();
    if (cpu_core_num <= 0) {
        printf(": Create message worker thread fail.\n");
        return false;
    }

    // 设置工作线程个数
    m_event_threads.reserve(cpu_core_num);

    // 批量创建工作线程
    std::generate_n(std::back_inserter(m_event_threads), cpu_core_num, [this]() {
        return std::thread(std::bind(&CMsgWorkerManager::event_thread_daemon, this));
    });

    printf(": Create message worker thread succ. worker size : %d\n", (int)m_event_threads.size());
    return true;
}

void CMsgWorkerManager::Stop()
{
    if (!m_event_threads.size()) {
        return;
    }

    // 通知中断线程
    m_event_stop = true;

    // 等待线程结束
    for (auto& event_thread : m_event_threads) {
        if (event_thread.joinable()) {
            event_thread.join();
        }
    }

    // 释放线程资源
    m_event_threads.clear();
    m_event_stop = false;

    printf(": Destory message worker thread succ.\n");
}

void CMsgWorkerManager::event_thread_daemon()
{
    while (!m_event_stop) {
        MsgQueueManager::instance().RpopMessage([this](redisReply* reply) {
        });
    }
}
