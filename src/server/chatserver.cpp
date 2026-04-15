#include"chatserver.hpp"
#include"chatservice.hpp"
#include<functional>
#include<nlohmann/json.hpp>
using namespace std;
using namespace placeholders; // 使用占位符命名空间，方便绑定回调函数
using json=nlohmann::json; // 使用nlohmann库的json命名空间，方便处理JSON数据

// 构造函数，初始化TcpServer对象和EventLoop指针
ChatServer::ChatServer(EventLoop *loop, // 事件循环对象的指针
        const InetAddress& listenAddr, //Ip地址和端口号
        const string& nameArg) // 服务器的名字
        :_server(loop,listenAddr,nameArg),_loop(loop)
{   
    // 给服务器注册用户连接的创建和断开回调函数
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
    // 给服务器注册用户读写事件的回调函数
    _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
    // 设置服务器的线程数量，muduo库会自动分配线程处理用户的连接和读写事件
    _server.setThreadNum(4);
}

void ChatServer::start()
{
    _server.start(); // 启动服务器，开始监听用户的连接请求
}

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if(!conn->connected())
    {
        // 调用ChatServer类的静态成员函数，处理客户端异常关闭的情况
        ChatService::instance()->clientCloseException(conn); 
        conn->shutdown(); // 关闭连接，释放资源
    }
}

void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time)
{
    string buf=buffer->retrieveAllAsString(); // 从缓冲区中读取数据并转换为字符串
    json js=json::parse(buf); // 将字符串解析为JSON对象,数据的反序列化

    // 完全解耦服务器与业务逻辑的处理，服务器只负责接收和发送数据，业务逻辑的处理交给外部的回调函数来完成
    // 通过js['msg_type']获取消息类型，根据不同的消息类型进行不同的处理

     // 获取消息类型对应的处理函数对象，并调用该函数对象来处理消息
    auto msgHandler=ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn,js,time); // 调用消息处理函数对象，传入连接对象、JSON对象和时间戳作为参数
}