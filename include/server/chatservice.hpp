#pragma once

#include<unordered_map> // 引入unordered_map头文件，提供哈希表数据结构
#include<functional> // 引入functional头文件，提供函数对象和绑定功能
#include<mutex> // 引入mutex头文件，提供互斥锁功能
using namespace std;

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

#include<nlohmann/json.hpp> // 引入nlohmann/json头文件，提供JSON数据处理功能
using json=nlohmann::json; // 使用nlohmann库的json命名空间，方便处理JSON数据

#include"UserModel.hpp" // 引入UserModel头文件，提供用户数据操作功能
#include"OfflineMessageModel.hpp" // 引入OfflineMessageModel头文件，提供离线消息数据操作功能
#include"friendModel.hpp" // 引入friendModel头文件，提供好友数据操作功能
#include"groupModel.hpp" // 引入groupModel头文件，提供群组数据操作功能
#include"redis.hpp" // 引入redis头文件，提供Redis数据库操作功能


// 定义MsgHandler类型，表示处理消息的函数对象，接受TcpConnectionPtr、json对象和Timestamp作为参数)
using MsgHandler=std::function<void(const TcpConnectionPtr &conn,const json &js,Timestamp)>; 


// 聊天服务器业务类，负责处理用户的注册、登录、消息发送等业务逻辑
class ChatService
{
public:
    static ChatService* instance(); // 获取ChatService单例对象的静态方法

    // 处理用户登录的业务逻辑
    void login(const TcpConnectionPtr &conn,const json &js,Timestamp time);
    // 处理用户注册的业务逻辑
    void reg(const TcpConnectionPtr &conn,const json &js,Timestamp time);
    // 处理用户注销的业务逻辑
    void logout(const TcpConnectionPtr &conn,const json &js,Timestamp time);
    // 处理单人聊天的业务逻辑
    void oneChat(const TcpConnectionPtr &conn,const json &js,Timestamp time);
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 添加好友业务的实现
    void addFriend(const TcpConnectionPtr &conn,const json &js,Timestamp time);

    // 创建群组业务的实现
    void createGroup(const TcpConnectionPtr &conn,const json &js,Timestamp time);
    // 加入群组业务的实现
    void addGroup(const TcpConnectionPtr &conn,const json &js,Timestamp time);
    // 群聊业务的实现
    void groupChat(const TcpConnectionPtr &conn,const json &js,Timestamp time);
    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int userid,string msg);

    // 处理客户端异常关闭的情况
    void clientCloseException(const TcpConnectionPtr &conn);
    // 异常退出后，重置用户的状态信息
    void reset();

private:
    ChatService(); // 私有化构造函数，禁止外部创建ChatService对象

    // 消息类型与处理函数的映射表，key是消息类型，value是对应的处理函数对象
    unordered_map<int,MsgHandler> _msgHandlerMap;

    // 用户数据操作对象，提供对用户数据的增删改查功能
    UserModel _userModel;
    // 离线消息数据操作对象，提供对离线消息数据的增删改查功能
    OfflineMsgModel _offlineMsgModel;
    // 好友数据操作对象，提供对好友数据的增删改查功能
    FriendModel _friendModel;
    // 群组数据操作对象，提供对群组数据的增删改查功能
    GroupModel _groupmodel;


    // 存储在线用户的通信连接
    unordered_map<int,TcpConnectionPtr> _userConnectionMap;
    // 定义互斥锁，保证对_userConnectionMap的访问线程安全
    mutex _connmutex;    

    // Redis操作对象，提供对Redis数据库的连接和操作功能
    Redis _redis;
};