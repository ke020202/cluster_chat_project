#include"chatservice.hpp"
#include"public.hpp"
#include<muduo/base/Logging.h> // 引入muduo库的Logging头文件，提供日志记录功能
#include<vector>
using namespace muduo; // 使用muduo命名空间
using namespace std;

ChatService* ChatService::instance()
{
    // 定义一个静态的chatService对象，确保在整个程序中只有一个实例存在
    static ChatService service; 
    return &service; // 返回chatService对象的地址，即单例对象的指针
}

//构成函数，初始化消息类型与处理函数的映射表
ChatService::ChatService()
{
    // 将登录消息类型与login函数绑定，并插入到消息处理函数映射表中
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)}); 
    // 将注销消息类型与clientCloseException函数绑定，并插入到消息处理函数映射表中
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::logout, this, _1, _2, _3)});
    // 将注册消息类型与reg函数绑定
    _msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    // 将单人聊天消息类型与oneChat函数绑定
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    // 将添加好友消息类型与addFriend函数绑定
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});
    // 将创建群组消息类型与createGroup函数绑定
    _msgHandlerMap.insert({CREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,_1,_2,_3)});
    // 将加入群组消息类型与addGroup函数绑定
    _msgHandlerMap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    // 将群聊消息类型与groupChat函数绑定
    _msgHandlerMap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});

    // 连接redis服务器
    if(_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    _userModel.resetState(); // 调用UserModel对象的resetState方法，重置用户状态信息
}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 根据消息类型msgid从映射表中获取对应的处理函数对象，并返回
    auto it=_msgHandlerMap.find(msgid); // 在消息处理函数映射表中查找msgid对应的处理函数对象
    if(it==_msgHandlerMap.end()) // 如果没有找到对应的处理函数对象
    {        
        // 返回一个默认的处理函数对象，输出错误日志，提示消息类型未处理
        return [=](const TcpConnectionPtr &conn,const json &js,Timestamp time)
        {
            // 记录错误日志，提示消息类型未找到
            LOG_ERROR <<"msgid:"<< msgid << "not found!";
        };

    }else return _msgHandlerMap[msgid]; 
}

//处理用户登录的业务逻辑 id和password
void ChatService::login(const TcpConnectionPtr &conn,const json &js,Timestamp time)
{
    int id=js["id"].get<int>(); // 从JSON对象中获取用户ID，并转换为整数类型
    string password=js["password"];

    User user=_userModel.query(id); // 根据用户ID查询用户信息
    if(user.getId()!=-1&&user.getPassword()==password) // 登陆成功
    {
        if(user.getState()=="online") // 如果用户已经在线，不允许重复登录
        {
            json response;
            response["msgid"]=LOGIN_MSG_ACK; // 设置响应消息类型为登录消息响应
            response["errno"]=2; // 设置响应消息中的错误码为2，表示用户已经在线
            response["errmsg"]="user is already online,please change account"; // 设置响应消息中的错误信息，提示用户已经在线
            conn->send(response.dump()); // 将响应消息转换为字符串并发送给客户端
            return;
        }
        // 登陆成功，记录用户连接信息,将用户ID和对应的通信连接插入到在线用户连接映射表中
        {
            lock_guard<mutex> lock(_connmutex);
            _userConnectionMap.insert({id,conn}); 
        }

        // id登陆成功后，向redis订阅channel(id)
        _redis.subscribe(id);      


        //登陆成功，更新用户状态为online
        user.setState("online");
        _userModel.updateState(user); // 更新用户信息到数据库中

        json response; 
        response["msgid"]=LOGIN_MSG_ACK;
        response["errno"]=0;
        response["id"]=user.getId();
        response["name"]=user.getName();

        // 查询用户是否有离线消息
        vector<string> vec=_offlineMsgModel.query(id);
        if(!vec.empty())
        {
            response["offlinemsg"]=vec; // 将离线消息列表添加到响应消息中
            _offlineMsgModel.remove(id); // 删除用户的离线消息
        }

        // 查询该用户的好友信息并返回
        vector<User> userVec=_friendModel.query(id);
        if(!userVec.empty())
        {
            vector<string> friendVec;
            for(User &user:userVec)
            {
                json js;
                js["id"]=user.getId();
                js["name"]=user.getName();
                js["state"]=user.getState();
                friendVec.push_back(js.dump());
            }
            response["friends"]=friendVec; // 将好友信息列表添加到响应消息中
        }

        // 查询用户的群组信息
        vector<Group> groupuserVec=_groupmodel.queryGroups(id);
        if(!groupuserVec.empty())
        {
            // group[{groupid:[XXX,XXX,XXX]},{groupid:[XXX,XXX,XXX]}]
            vector<string> groupVec;
            for(Group &group:groupuserVec)
            {
                json groupjs;
                groupjs["id"]=group.getId();
                groupjs["name"]=group.getName();
                groupjs["desc"]=group.getDesc();
                vector<string> userVec;
                for(GroupUser &user:group.getUsers())
                {
                    json userjs;
                    userjs["id"]=user.getId();
                    userjs["name"]=user.getName();
                    userjs["state"]=user.getState();
                    userjs["role"]=user.getRole();
                    userVec.push_back(userjs.dump());
                }
                groupjs["users"]=userVec;
                groupVec.push_back(groupjs.dump());
            }
            response["groups"]=groupVec; // 将群组信息列表添加到响应消息中
        }

        conn->send(response.dump()); // 将响应消息转换为字符串并发送给客户端
    }else
    {
        json response;
        response["msgid"]=LOGIN_MSG_ACK;
        response["errno"]=1; // 设置响应消息中的错误码为1，表示登录失败
        response["errmsg"]="id or password is invalid"; // 设置响应消息中的错误信息，提示账号或密码错误
        conn->send(response.dump()); 
    }
} /* test json:
  {"msgid":1,"id":1,"password":"123456"}  //返回正确
  {"msgid":1,"id":8,"password":"123456"}  //返回错误
*/

//处理用户注册的业务逻辑 name password
void ChatService::reg(const TcpConnectionPtr &conn,const json &js,Timestamp time)
{
    string name=js["name"];
    string password=js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);

    bool state=_userModel.insert(user);
    if(state) // 如果注册成功，返回注册成功的响应给客户端
    {
        json response;
        response["msgid"]=REG_MSG_ACK; // 设置响应消息类型为注册消息响应
        response["errno"]=0; // 设置响应消息中的错误码为0，表示没有错误
        response["id"]=user.getId(); // 设置响应消息中的用户ID
        conn->send(response.dump()); // 将响应消息转换为字符串并发送给客户端
    }else // 失败
    {
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["errno"]=1; // 设置响应消息中的错误码为1，表示注册失败
        response["id"]=user.getId();
        response["errmsg"]=name+" is already exist"; // 设置响应消息中的错误信息，提示用户已存在
        conn->send(response.dump()); 
    }
} /* test json:
  {"msgid":3,"name":"Alice","password":"666666"}
*/

// 处理用户注销的业务逻辑
void ChatService::logout(const TcpConnectionPtr &conn,const json &js,Timestamp time)
{
    int userid=js["id"].get<int>(); // 获取用户Id
    {
        lock_guard<mutex> lock(_connmutex);
        auto it=_userConnectionMap.find(userid); // 在在线用户连接映射表中查找用户ID对应的连接对象
        if(it!=_userConnectionMap.end()) // 如果找到了对应的连接对象
        {
            _userConnectionMap.erase(it); // 从在线用户连接映射表中删除用户的连接信息
        }
    }
    // 用户注销，在redis中取消订阅channel(userid)
    _redis.unsubscribe(userid);

    User user;
    user.setId(userid); // 设置用户ID
    user.setState("offline"); // 设置用户状态为离线
    _userModel.updateState(user); // 更新用户信息到数据库中
}

// 处理客户端异常关闭的情况
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connmutex);        
        for(auto it=_userConnectionMap.begin();it!=_userConnectionMap.end();++it)
        {
            if(it->second==conn) // 如果找到了对应的连接对象
            {
                // 更新用户的状态信息为offline
                user.setId(it->first); // 获取用户ID
                // 从map表中删除用户的连接信息
                _userConnectionMap.erase(it);
                break;
            }
        }
    }
    // 用户异常退出，在redis中取消订阅channel(userid)
    _redis.unsubscribe(user.getId());

    if(user.getId()!=-1) // 如果找到了对应的用户ID
    {
        user.setState("offline"); // 设置用户状态为离线
        _userModel.updateState(user); // 更新用户信息到数据库中
    }
}

// 处理单人聊天的业务逻辑
void ChatService::oneChat(const TcpConnectionPtr &conn,const json &js,Timestamp time)
{
    int toid=js["to"].get<int>();
    {
        lock_guard<mutex> lock(_connmutex);
        auto it=_userConnectionMap.find(toid);
        if(it!=_userConnectionMap.end()) // 在线，转发消息
        {
            it->second->send(js.dump()); // 将消息转换为字符串并发送给目标用户的连接对象
            return;
        }
    }

    // 查询toid是否在线
    User user=_userModel.query(toid);
    if(user.getState()=="online") // 不在一个服务器登录，转发消息到toid所在的服务器
    {
        _redis.publish(toid,js.dump()); // 向redis指定的通道toid发布消息js
        return;
    }

    // 不在线，存储离线消息
    _offlineMsgModel.insert(toid,js.dump()); 

}/* test json:
  {"msgid":1,"id":1,"password":"123456"}  //登录
  {"msgid":1,"id":2,"password":"666666"}  //登录
  {"msgid":5,"id":1,"from":"zhangsan","to":2,"msg":"hello222"} //单人聊天
  {"msgid":5,"id":2,"from":"lisi","to":1,"msg":"very good！"} //单人聊天
*/

// 添加好友业务的实现 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn,const json &js,Timestamp time)
{
    int userid=js["id"].get<int>(); // 获取用户ID
    int friendid=js["friendid"].get<int>(); // 获取好友ID

    _friendModel.insert(userid,friendid); // 在数据库中添加好友关系
}/*
{"msgid":6,"id":1,"friendid":2} //添加好友

*/

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn,const json &js,Timestamp time)
{
    int userid=js["id"].get<int>();
    string name=js["groupname"];
    string desc=js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1,name,desc);
    if(_groupmodel.createGroup(group))
    {
         // 将创建者加入群组，角色为creator
        _groupmodel.addGroup(userid,group.getId(),"creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn,const json &js,Timestamp time)
{
    int userid=js["id"].get<int>();
    int groupid=js["groupid"].get<int>();

    // 将用户加入群组，角色为normal
    _groupmodel.addGroup(userid,groupid,"normal");
}

// 群聊业务
void ChatService::groupChat(const TcpConnectionPtr &conn,const json &js,Timestamp time)
{
    int userid=js["id"].get<int>();
    int groupid=js["groupid"].get<int>();

    // 查询群组用户列表，获取出自己以外的所有人的id    
    vector<int> useridVec=_groupmodel.queryGroupUsers(userid,groupid);

    lock_guard<mutex> lock(_connmutex);
    for(int id:useridVec)
    {        
        auto it=_userConnectionMap.find(id);
        if(it!=_userConnectionMap.end())
        {
            // 转发消息
            it->second->send(js.dump());
        }else
        {
            User user=_userModel.query(id);
            if(user.getState()=="online") // 其它服务器在线，转发消息到用户所在的服务器
            {
                _redis.publish(id,js.dump());
            }else
            {
                // 存储离线消息
                _offlineMsgModel.insert(id,js.dump());
            }

        }
    }
}

// 从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid,string msg)
{
    json js=json::parse(msg.c_str()); // 将消息字符串解析为JSON对象

    lock_guard<mutex> lock(_connmutex); // 加锁，保证线程安全
    auto it=_userConnectionMap.find(userid); // 在在线用户连接映射表中查找用户ID对应的连接对象
    if(it!=_userConnectionMap.end()) // 如果找到了对应的连接对象
    {
        it->second->send(js.dump()); // 将消息转换为字符串并发送给用户的连接对象
        return;
    }
    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid,js.dump());
}