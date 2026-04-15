#pragma once

#include<muduo/net/TcpServer.h> //TcpServer是muduo库中提供的类，用于创建TCP服务器，监听客户端连接，并处理网络事件。
#include<muduo/net/EventLoop.h> //EventLoop是muduo库中提供的类，用于管理事件循环，处理网络事件和定时器事件。
using namespace muduo; // 使用muduo命名空间
using namespace muduo::net; // 使用muduo网络库的命名空间

// 聊天服务器主类，负责创建和管理TCP服务器，处理客户端连接和消息事件。
class ChatServer
{
public:
    //初始化聊天服务器对象，接受一个EventLoop指针、一个InetAddress对象和一个字符串作为参数。
    ChatServer(EventLoop *loop, // 事件循环对象的指针
        const InetAddress& listenAddr, //Ip地址和端口号
        const string& nameArg); // 服务器的名字
    
    // 启动服务器
    void start();

private:
    // 专门处理用户的连接创建和断开 epoll listenfd accept
    void onConnection(const TcpConnectionPtr &conn);
    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr &conn,Buffer *buffer,Timestamp time);


    TcpServer _server; // TcpServer对象，用于创建和管理TCP服务器。
    EventLoop *_loop; // EventLoop对象指针，用于管理事件循环，处理网络事件和定时器事件。
};