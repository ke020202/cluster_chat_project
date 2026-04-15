#include<nlohmann/json.hpp>
using json=nlohmann::json;

#include<iostream>
#include<thread>
#include<string>
#include<ctime>
#include<vector>
#include<chrono>
using namespace std;

#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#include"group.hpp"
#include"public.hpp"
#include"user.hpp"

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前系统登陆用户的好友列表的信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 显示当前登录用户的基本信息
void showCurrentUserData();
// 获取系统时间（聊天信息需要添加时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 接收线程
void readTaskHandler(int clientfd);

// 控制聊天界面
bool isMainMenuRunning=false; // 主聊天页面是否正在运行的标志

// 聊天客户端程序实现，main线程用于发送线程，子线程用于接收线程
int main(int argc,char **argv)
{
    if(argc<3)
    {
        cerr<<"command invalid! example: ./ChatClient 127.0.0.1 6000"<<endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip=argv[1];
    uint16_t port=atoi(argv[2]);

    // 创建client端的socket
    int clientfd=socket(AF_INET,SOCK_STREAM,0); // AF_INET表示ipv4协议，SOCK_STREAM表示TCP协议
    if(clientfd==-1) // socket函数执行失败，返回-1
    {
        cerr<<"socket create error!"<<endl;
        exit(-1);
    }

    // 填写client需要连接的server端的地址信息ip+port
    sockaddr_in server;
    memset(&server,0,sizeof(server)); // 将server地址信息结构体变量清零

    server.sin_family=AF_INET; // ipv4协议
    server.sin_port=htons(port); // 将port转换为网络字节序
    server.sin_addr.s_addr=inet_addr(ip); // 将ip地址转换为网络字节序

    // client连接server
    if(connect(clientfd,(sockaddr *)&server,sizeof(server))==-1) // connect函数执行失败，返回-1
    {
        cerr<<"connect server error!"<<endl;
        close(clientfd);
        exit(-1);
    }

    // main线程用于接收用户输入，负责发送数据
    while(1)
    {
        cout <<"=================welcome to chat system================="<<endl;
        cout<<"1. login"<<endl;
        cout<<"2. register"<<endl;
        cout<<"3. quit"<<endl;
        cout<<"========================================================"<<endl;
        cout<<"please input your choice: "<<endl;
        int choice=0;
        cin>>choice; cin.get(); // 读取用户输入的选择，并清除输入缓冲区中的换行符

        switch(choice)
        {
            case 1: // login业务
            {
                int id=0;
                char pwd[50]={0};
                cout<<"please input user id: "<<endl;
                cin>>id; cin.get();
                cout<<"please input password: "<<endl;
                cin.getline(pwd,50); // 读取用户输入的密码，并存储在pwd数组中

                json js;
                js["msgid"]=LOGIN_MSG; // 设置消息类型为登录消息
                js["id"]=id; // 设置消息中的用户ID字段
                js["password"]=pwd; // 设置消息中的密码字段
                string request=js.dump(); // 将JSON对象转换为字符串格式，准备发送给服务器

                if(send(clientfd,request.c_str(),strlen(request.c_str())+1,0)==-1) 
                {// 发送消息失败，返回-1
                    cerr<<"send login msg error!"<<endl;
                }else
                {
                    char buffer[1024]={0};
                    if(recv(clientfd,buffer,1024,0)==-1)
                    {
                        cerr<<"recv login response error!"<<endl;
                    }else 
                    {
                        json responsejs=json::parse(buffer); // 将接收到的字符串消息解析为JSON对象
                        
                        if(responsejs["errno"].get<int>()!=0)
                        {
                            cerr<<responsejs["errmsg"].get<string>()<<endl; // 登录失败，显示错误信息
                        }else
                        {                            
                            // 登录成功，显示登录用户的基本信息
                            g_currentUser.setId(responsejs["id"].get<int>());
                            g_currentUser.setName(responsejs["name"].get<string>());
                            
                            // 记录当前用户的好友列表信息
                            if(responsejs.contains("friends"))
                            {
                                g_currentUserFriendList.clear(); // 先清空
                                vector<string> vec=responsejs["friends"];
                                for(string &str:vec)
                                {
                                    json js=json::parse(str);
                                    User user;
                                    user.setId(js["id"].get<int>());
                                    user.setName(js["name"].get<string>());
                                    user.setState(js["state"].get<string>());
                                    g_currentUserFriendList.push_back(user);
                                }
                            }

                            // 记录当前用户的群组列表信息                            
                            if(responsejs.contains("groups"))
                            {
                                g_currentUserGroupList.clear(); // 先清空
                                vector<string> vec1=responsejs["groups"];
                                for(string &groupstr:vec1)
                                {
                                    json groupjs=json::parse(groupstr);
                                    Group group;
                                    group.setId(groupjs["id"].get<int>());
                                    group.setName(groupjs["name"].get<string>());
                                    group.setDesc(groupjs["desc"].get<string>());
                                    vector<string> vec2=groupjs["users"];
                                    for(string &userstr:vec2)
                                    {
                                        json userjs=json::parse(userstr);
                                        GroupUser user;
                                        user.setId(userjs["id"].get<int>());
                                        user.setName(userjs["name"].get<string>());
                                        user.setState(userjs["state"].get<string>());
                                        user.setRole(userjs["role"].get<string>());
                                        group.getUsers().push_back(user);
                                    }
                                    g_currentUserGroupList.push_back(group);               
                                }
                            }
                            // 显示当前登录用户的基本信息
                            showCurrentUserData();

                            // 显示当前用户的离线信息（个人聊天或群发信息）
                            if(responsejs.contains("offlinemsg"))
                            {
                                vector<string> vec=responsejs["offlinemsg"];
                                for(string &str:vec)
                                {
                                    json js=json::parse(str);
                                    int msgtype=js["msgid"].get<int>();
                                    if(msgtype==ONE_CHAT_MSG)
                                    {
                                        cout<<js["time"].get<string>()<<" ["<<js["id"].get<int>()<<"] "<<js["name"].get<string>()<<": "<<js["msg"].get<string>()<<endl;
                                    }else if(msgtype==GROUP_CHAT_MSG)
                                    {
                                        cout<<"群消息["<<js["groupid"].get<int>()<<"]: "<<js["time"].get<string>()<<" ["<<js["id"].get<int>()<<"] "<<js["name"].get<string>()<<": "<<js["msg"].get<string>()<<endl;
                                    }
                                }
                            }
                            
                            static int readThreadCount=0; // 记录接收线程的数量
                            if(readThreadCount==0)  // 如果还没有接收线程，启动接收线程，接收服务器转发的消息
                            {
                                // 登录成功，启动接收线程，接收服务器转发的消息
                                std::thread readTask(readTaskHandler,clientfd);
                                readTask.detach(); // 将接收线程分离，使其在后台运行，不阻塞主线程继续执行其他任务
                                readThreadCount++;
                            }
                            
                            // 设置主聊天页面正在运行的标志为true
                            isMainMenuRunning=true; 
                            mainMenu(clientfd); // 进入主聊天页面
                        }
                    }
                }
            }
            break;
            case 2:
            {
                char name[50]={0};
                char pwd[50]={0};
                cout<<"please input user name: "<<endl;
                cin.getline(name,50);
                cout<<"please input password: "<<endl;
                cin.getline(pwd,50);

                json js;
                js["msgid"]=REG_MSG; // 设置消息类型为注册消息
                js["name"]=name; // 设置消息中的用户名字段
                js["password"]=pwd; // 设置消息中的密码字段
                string request=js.dump(); // 将JSON对象转换为字符串格式，准备发送给服务器
                if(send(clientfd,request.c_str(),strlen(request.c_str())+1,0)==-1)
                {
                    cerr<<"send reg msg error!"<<request<<endl;
                }else
                {
                    char buffer[1024]={0};
                    int len=recv(clientfd,buffer,1024,0);
                    if(len==-1)
                    {
                        cerr<<"recv reg response error!"<<endl;
                    }else
                    {
                        json responsejs=json::parse(buffer);
                        // 注册失败，服务器返回的errno不为0，表示注册失败
                        if(0 != responsejs["errno"].get<int>()) 
                        {
                            // 注册失败，显示错误信息
                            cerr<<name<<" is already exist, register error!"<<endl;
                        }else
                        {
                            cout<<name<<" register success, your userid is "<<responsejs["id"].get<int>()<<endl;
                        }
                    }

                }
            }
            break;
            case 3: // quit业务
            {
                close(clientfd); // 关闭客户端socket连接
                exit(0);
            }
            default:
            {
                cerr<<"invalid choice!"<<endl;
                break;
            }
        }
    }
}

string getCurrentTime()
{
    return "";
}

// 显示当前登录用户的基本信息
void showCurrentUserData()
{
    cout<<"------------------login user------------------"<<endl;
    cout<<"current login user => id: "<<g_currentUser.getId()
        <<" name: "<<g_currentUser.getName()<<endl;
    
    if(!g_currentUserFriendList.empty())
    {
        cout<<"------------------friend list------------------"<<endl;
        for(User &user:g_currentUserFriendList)
        {
            cout<<"friend => id: "<<user.getId()
                <<" name: "<<user.getName()
                <<" state: "<<user.getState()<<endl;
        }
    }

    
    if(!g_currentUserGroupList.empty())
    {
        cout<<"------------------group list------------------"<<endl;
        for(Group &group:g_currentUserGroupList)
        {
            cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
            for(GroupUser &user:group.getUsers())
            {
                cout<<"group user => id: "<<user.getId()
                    <<" name: "<<user.getName()
                    <<" state: "<<user.getState()
                    <<" role: "<<user.getRole()<<endl;
            }
        }
    }
    cout<<"---------------------------------------------"<<endl;
}

// 接收线程
void readTaskHandler(int clientfd)
{
    while(1)
    {
        char buffer[1024]={0};
        int len=recv(clientfd,buffer,1024,0);
        /*
            len > 0：成功读到 len 字节数据 ✅
            len == 0：客户端正常断开连接 📤
            len == -1：读取发生错误 ❌
        */
        if(len==-1||len==0)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收chatserver转发的数据，反序列化生成json数据对象
        json js=json::parse(buffer);
        int msgtype=js["msgid"].get<int>();
        if(msgtype==ONE_CHAT_MSG)
        {
            cout<<js["time"].get<string>()<<" ["<<js["id"].get<int>()<<"] "<<js["name"].get<string>()<<": "<<js["msg"].get<string>()<<endl;
            continue;
        }else if(msgtype==GROUP_CHAT_MSG)
        {
            cout<<"群消息["<<js["groupid"].get<int>()<<"]: "<<js["time"].get<string>()<<" ["<<js["id"].get<int>()<<"] "<<js["name"].get<string>()<<": "<<js["msg"].get<string>()<<endl;
            continue;
        }
    }
}

void help(int clientfd=0,string command="");
void chat(int clientfd,string command);
void addfriend(int clientfd,string command);
void creategroup(int clientfd,string command);
void addgroup(int clientfd,string command);
void groupchat(int clientfd,string command);
void logout(int clientfd,string command);

// 系统支持的客户端命令列表
unordered_map<string,string> commandMap=
{
    {"help","显示所有支持的命令，格式 help"},
    {"chat","私聊消息，格式 chat:friendid:message"},
    {"addfriend","添加好友，格式 addfriend:friendid"},
    {"creategroup","创建群组，格式 creategroup:groupname:groupdesc"},
    {"addgroup","加入群组，格式 addgroup:groupid"},
    {"groupchat","群聊消息，格式 groupchat:groupid:message"},
    {"logout","注销，格式 logout"}
};
// 注册系统支持的客户端命令处理
unordered_map<string,function<void(int,string)>> commandHandlerMap=
{
    {"help",help},
    {"chat",chat},
    {"addfriend",addfriend},
    {"creategroup",creategroup},
    {"addgroup",addgroup},
    {"groupchat",groupchat},
    {"logout",logout}
};

//主聊天页面程序
void mainMenu(int clientfd)
{
    help();
    char buffer[1024]={0};
    while(isMainMenuRunning)
    {
        cin.getline(buffer,1024);
        string commandbuf(buffer);
        string command; // 存储命令
        int idx=commandbuf.find(":"); // 查找命令字符串中第一个冒号的位置
        if(idx==-1)
        {
            command=commandbuf; // 如果没有冒号，整个输入就是命令
        }else
        {
            command=commandbuf.substr(0,idx); // 提取冒号前的部分作为命令
        }
        auto it=commandHandlerMap.find(command); // 在命令处理函数映射中查找对应的命令
        if(it==commandHandlerMap.end())
        {
            cerr<<"invalid command!"<<endl;
            continue;
        }
        it->second(clientfd,commandbuf.substr(idx+1,commandbuf.size()-idx)); // 调用对应的命令处理函数，传递命令参数

    }
}

void help(int clientfd,string command)
{
    cout<<"show command list >>> "<<endl;
    for(auto &t:commandMap)
    {
        cout<<t.first<<" : "<<t.second<<endl;
    }
    cout<<endl;
}

void addfriend(int clientfd,string command)
{
    int friendid=atoi(command.c_str());
    json js;
    js["msgid"]=ADD_FRIEND_MSG;
    js["id"]=g_currentUser.getId();
    js["friendid"]=friendid;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"send addfriend msg error ->"<<buffer<<endl;
    }
}

void chat(int clientfd,string command)
{
    int idx=command.find(":");
    if(idx==-1)
    {
        cerr<<"chat command invalid!"<<endl;
        return;
    }
    int friendid=atoi(command.substr(0,idx).c_str());
    string message=command.substr(idx+1,command.size()-idx);
    json js;
    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["to"]=friendid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"send chat msg error ->"<<buffer<<endl;
    }
}
// {"creategroup","创建群组，格式 creategroup:groupname:groupdesc"},
void creategroup(int clientfd,string command)
{
    int idx=command.find(":");
    if(idx==-1)
    {
        cerr<<"creategroup command invalid!"<<endl;
        return;
    }
    string groupname=command.substr(0,idx);
    string groupdesc=command.substr(idx+1,command.size()-idx);
    json js;
    js["msgid"]=CREATE_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupname"]=groupname;
    js["groupdesc"]=groupdesc;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"send creategroup msg error ->"<<buffer<<endl;
    }
}

void addgroup(int clientfd,string command)
{
    int groupid=atoi(command.c_str());
    json js;
    js["msgid"]=ADD_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupid"]=groupid;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"send addgroup msg error ->"<<buffer<<endl;
    }
}
// groupchat:groupid:message
void groupchat(int clientfd,string command)
{
    int idx=command.find(":");
    if(idx==-1)
    {
        cerr<<"groupchat command invalid!"<<endl;
        return;
    }
    int groupid=atoi(command.substr(0,idx).c_str());
    string message=command.substr(idx+1,command.size()-idx);
    json js;
    js["msgid"]=GROUP_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["groupid"]=groupid;
    js["msg"]=message;
    js["time"]=getCurrentTime();
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"send groupchat msg error ->"<<buffer<<endl;
    }
}

void logout(int clientfd,string command)
{
    json js;
    js["msgid"]=LOGINOUT_MSG;
    js["id"]=g_currentUser.getId();
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1)
    {
        cerr<<"send logout msg error ->"<<buffer<<endl;
    }else
    {
        isMainMenuRunning=false; // 设置主聊天页面正在运行的标志为false，退出主聊天页面
    }
}