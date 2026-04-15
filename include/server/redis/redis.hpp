#pragma once

#include<hiredis/hiredis.h>
#include<functional>
#include<thread>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    //连接redis服务器
    bool connect();

    // 向redis指定的通道channel发布消息message
    bool publish(int channel,string message);

    // 向redis指定的通道channel订阅消息message
    bool subscribe(int channel);

    // 向resid指定的通道channel取消订阅消息message
    bool unsubscribe(int channel);

    // 专门接收订阅消息的线程函数
    void observer_channel_message();

    // 初始化向业务层上报通道消息的回调函数
    void init_notify_handler(function<void(int,string)> fn);
private:
    // hiredis同步上下文对象，负责publichh消息
    redisContext* _publish_context;

    // hiredis同步上下文对象，负责subscribe消息
    redisContext* _subcribe_context;
    
    // 业务层上报消息的回调函数
    function<void(int,string)> _notify_message_handler;
};