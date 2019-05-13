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
    // �¼������߳�
    void event_thread_daemon();

    // ȷ�� hiredis �������
    redisContext* ensure_redis_context();

private:
    // hiredis ��������
    unsigned int m_redis_request_count;

    // hiredis ����
    redisContext* m_redis_context;

    // hiredis ���ض���
    std::queue<redisReply*> m_redis_reply_queue;

    // �¼��߳�ID
    std::thread* m_event_thread;

    // �¼��̵߳��ź���
    std::mutex m_event_mutex;

    // �¼��������
    bool m_event_stop;

    // �¼��߳̽����ź�
    std::condition_variable m_event_cvar;

    // �¼��������ź�
    std::condition_variable m_event_consume;

    // �¼��������ź�
    std::condition_variable m_event_product;

    // �¼��������ź�
    std::condition_variable m_event_noempty;
};

typedef Container::singleton_default<CMsgQueueManager> MsgQueueManager;

#endif // !_MSG_QUEUE_MANAGER_H
