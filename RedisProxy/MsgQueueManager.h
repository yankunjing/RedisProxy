#ifndef _MSG_QUEUE_MANAGER_H
#define _MSG_QUEUE_MANAGER_H

#include <queue>              // std::queue
#include <thread>             // std::thread
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable, std::cv_status
#include <functional>         // std::function
#include "Singleton.h"

struct redisReply;
struct redisContext;
class CMsgQueueManager
{
public:
    CMsgQueueManager();
    ~CMsgQueueManager();

public:
    bool Start();
    void Stop();

    void RpopMessage(std::function<void(redisReply*)> callback);
    void LpushMessage(const char* topic, const char* data);

private:
    // 事件处理线程
    void event_thread_daemon();

    // 确保 hiredis 对象存在
    redisContext* ensure_redis_context();

private:
    // hiredis 请求数量
    unsigned int m_redis_request_count;

    // hiredis 对象
    redisContext* m_redis_context;

    // hiredis 返回对象
    std::queue<redisReply*> m_redis_reply_queue;

    // 事件线程ID
    std::thread* m_event_thread;

    // 事件线程的信号量
    std::mutex m_event_mutex;

    // 事件结束标记
    bool m_event_stop;

    // 事件线程结束信号
    std::condition_variable m_event_cvar;

    // 事件消费者信号
    std::condition_variable m_event_consume;

    // 事件生产者信号
    std::condition_variable m_event_product;

    // 事件消费者信号
    std::condition_variable m_event_noempty;
};

typedef Container::singleton_default<CMsgQueueManager> MsgQueueManager;

#endif // !_MSG_QUEUE_MANAGER_H
