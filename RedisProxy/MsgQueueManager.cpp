#include "MsgQueueManager.h"
#include <stdlib.h>
#include <hiredis.h>
#include <functional>
#include <chrono>

CMsgQueueManager::CMsgQueueManager()
{
}

CMsgQueueManager::~CMsgQueueManager()
{
}

bool CMsgQueueManager::Start()
{
    if (m_event_thread) {
        return true;
    }

    // 重置事件结束标记
    m_event_stop = false;

    // 创建事件处理线程
    m_event_thread = new std::thread(std::bind(&CMsgQueueManager::event_thread_daemon, this));
    if (!m_event_thread) {
        printf(": Create message queue thread fail.\n");
        return false;
    }

    printf(": Create message queue thread succ.\n");
    return true;
}

void CMsgQueueManager::Stop()
{
    if (!m_event_thread) {
        return;
    }

    // 通知线程结束
    m_event_stop = true;
    m_event_product.notify_all();

    // 等待线程结束
    if (m_event_thread->joinable()) {
        m_event_thread->join();
    }

    // 释放线程资源
    delete m_event_thread;
    m_event_thread = nullptr;

    // 通知所有消费者
    m_event_consume.notify_all();
    printf(": Destory message queue thread succ.\n");
}

void CMsgQueueManager::RpopMessage(std::function<void(redisReply*)> callback)
{
    std::unique_lock<std::mutex> lock(m_event_mutex);
    if (m_event_stop) {
        //printf(": m_event_consume.stop\n");

        // 避免长时间占用线程
        std::this_thread::yield();
        return;
    }

    m_redis_request_count++;
    //printf(">>> m_redis_request_count need consum %d\n", m_redis_request_count);

    if (m_redis_reply_queue.empty()) {
        // 通知生产者工作
        m_event_product.notify_one();
        //printf(": m_event_product.notify_one\n");

        // 等待生产者消息
        //printf("thread[%08u] m_event_consume.wait\n", std::this_thread::get_id());
        m_event_consume.wait(lock);
        //printf("thread[%08u] m_event_consume.work\n", std::this_thread::get_id());
    }

    m_redis_request_count--;
    //printf(">>> m_redis_request_count need product %d\n", m_redis_request_count);

    m_event_noempty.notify_one();
    //printf(": m_event_noempty.notify_one\n");

    if (m_redis_reply_queue.empty()) {
        // 避免长时间占用线程
        //printf(": m_event_consume.empty\n");
        return;
    }

    //printf("<<< m_redis_reply_queue size consume %d\n", m_redis_reply_queue.size());
    redisReply* reply = m_redis_reply_queue.front();
    m_redis_reply_queue.pop();
    if (!reply) {
        return;
    }

    callback(reply);
    freeReplyObject(reply);
}

void CMsgQueueManager::LpushMessage(const char* topic, const char* data)
{
    redisContext* redis_context = ensure_redis_context();
    if (!redis_context) {
        ////---- todo: record lpush data
        return;
    }

    redisReply* reply = (redisReply*)redisCommand(m_redis_context, "LPUSH %s %s", topic, data);
    if (!reply) {
        return;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        return;
    }

    freeReplyObject(reply);
}

void CMsgQueueManager::event_thread_daemon()
{
    while (!m_event_stop) {
        // 争夺锁资源
        std::unique_lock<std::mutex> lock(m_event_mutex);

        // 没有客源
        if (m_redis_request_count <= 0) {
            //printf(": m_event_product.wait\n");
            m_event_product.wait(lock);
        }

        if (m_event_stop) {
            //printf(": m_event_product.stop\n");
            break;
        }

        // printf(": m_event_product.work\n");
        redisContext* redis_context = ensure_redis_context();
        if (!redis_context) {
            if (m_event_stop) {
                // printf(": m_event_product.stop\n");
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(3));
            // printf(": redis_context.reconnect\n");
            continue;
        }

        /* PING server */
        redisReply* reply = (redisReply*)redisCommand(m_redis_context, "PING");
        if (!reply) {
            continue;
        }

        m_redis_reply_queue.push(reply);
        //printf(">>> m_redis_reply_queue size product %d\n", m_redis_reply_queue.size());

        static int total = 0;
        static auto start = std::chrono::system_clock::now();
        static int sum = 10000;

        total++;
        if (total % sum == 0) {
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - start).count();
            printf(">>>>>>>> total = %d, cost = %f\n", sum, 1.0 * diff / 1000000);
            start = std::chrono::system_clock::now();
        }

        m_event_consume.notify_one();
        //printf(": m_event_consume.notify_one\n");

        m_event_noempty.wait(lock);
        //printf(": m_event_noempty.wait\n");
    }
}

redisContext* CMsgQueueManager::ensure_redis_context()
{
    if (m_redis_context) {
        return m_redis_context;
    }

    m_redis_context = redisConnect("127.0.0.1", 6379);
    if (!m_redis_context) {
        printf(": Connection error: can't allocate redis context\n");
        return nullptr;
    }

    if (m_redis_context->err) {
        printf(": Connection error: %s\n", m_redis_context->errstr);
        redisFree(m_redis_context);
        m_redis_context = nullptr;
        return nullptr;
    }

    redisReply* reply = (redisReply*)redisCommand(m_redis_context, "PING");
    if (!reply) {
        printf(": Connection error: can't auth redis context\n");
        redisFree(m_redis_context);
        m_redis_context = nullptr;
        return nullptr;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        freeReplyObject(reply);
        printf(": Connection error: auth redis fail\n");
        redisFree(m_redis_context);
        m_redis_context = nullptr;
        return nullptr;
    }

    freeReplyObject(reply);
    printf(": Connection: auth redis succ\n");

    /* return the context */
    return m_redis_context;
}
