#include"redis.hpp"
#include<iostream>
using namespace std;

Redis::Redis()
{
    _publish_context=nullptr;
    _subcribe_context=nullptr;
}

Redis::~Redis()
{
    if(_publish_context!=nullptr)
    {
        redisFree(_publish_context);
    }
    if(_subcribe_context!=nullptr)
    {
        redisFree(_subcribe_context);
    }
}

bool Redis::connect()
{
    // 负责publish发布消息的上下文连接
    _publish_context=redisConnect("127.0.0.1",6379);
    if(_publish_context==nullptr)
    {
        cerr<<"connect redis failed!"<<endl;
        return false;
    }

    // 负责subscribe订阅消息的上下文连接
    _subcribe_context=redisConnect("127.0.0.1",6379);
    if(_subcribe_context==nullptr)
    {
        cerr<<"connect redis failed!"<<endl;
        return false;
    }

    // 启动一个线程，专门接收订阅消息
    thread t([&](){
        observer_channel_message();
    });
    t.detach();

    cout<<"connect redis-server success!"<<endl;
    return true;
}

// 向redis指定的通道channel发布消息message
bool Redis::publish(int channel,string message)
{
    redisReply* reply=(redisReply*)redisCommand(_publish_context,"PUBLISH %d %s",channel,message.c_str());
    if(reply==nullptr)
    {
        cerr<<"publish command failed!"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

// 向redis指定的通道channel订阅消息message
bool Redis::subscribe(int channel)
{
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
    // 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
    // 只负责发送命令，不阻塞接收redis server响应消息，否则和notifyMsg线程抢占响应资源
    if(redisAppendCommand(this->_subcribe_context,"SUBSCRIBE %d",channel)==REDIS_ERR)
    {
        cerr<<"subscribe command failed!"<<endl;
        return false;
    }
    // redisBufferWrite可以循环发送缓冲区，直至缓冲区数据发送完毕
    int done=0;
    while(!done)
    {
        if(redisBufferWrite(this->_subcribe_context,&done)==REDIS_ERR)
        {
            cerr<<"subscribe command failed!"<<endl;
            return false;
        }
    }
    // redisGetReply 函数会阻塞等待redis server的响应消息，直到收到响应消息才会返回
    return true;
}

// 向resid指定的通道channel取消订阅消息message
bool Redis::unsubscribe(int channel)
{
    if(redisAppendCommand(this->_subcribe_context,"UNSUBSCRIBE %d",channel)==REDIS_ERR)
    {
        cerr<<"unsubscribe command failed!"<<endl;
        return false;
    }
    int done=0;
    while(!done)
    {
        if(redisBufferWrite(this->_subcribe_context,&done)==REDIS_ERR)
        {
            cerr<<"unsubscribe command failed!"<<endl;
            return false;
        }
    }
    return true;
}

// 专门接收订阅消息的线程函数
void Redis::observer_channel_message()
{
    redisReply* reply=nullptr;
    while(redisGetReply(this->_subcribe_context,(void**)&reply)==REDIS_OK)
    {
        // 订阅收到的消息是一个带三元素的数组
        /*
        1. "message"       // 消息类型
        2. "频道号"        // 比如 "1"、"2"、"10086"
        3. "消息内容"      // 真正的数据
        */
        if(reply!=nullptr&&reply->element[2]!=nullptr&&reply->element[2]->str!=nullptr)
        {
            // 通过回调函数把消息上报给业务层
            _notify_message_handler(atoi(reply->element[1]->str),reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr<<"observer_channel_message function redisGetReply failed!"<<endl;
}

void Redis::init_notify_handler(function<void(int,string)> fn)
{
    this->_notify_message_handler=fn;
}