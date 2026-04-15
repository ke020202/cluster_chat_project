#include"chatserver.hpp"
#include"chatservice.hpp"
#include<iostream>
#include<signal.h>
using namespace std;

// 处理ctrl+c信号，重置user的状态信息
void resetHandler(int)
{
    ChatService::instance()->reset(); // 调用ChatService单例对象的reset方法，重置用户的状态信息
    exit(0); // 退出程序
}
int main(int argc,char* argv[])
{
    if(argc<3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }
    char *ip=argv[1]; // 从命令行参数中获取IP地址
    uint16_t port=atoi(argv[2]); // 从命令行参数中获取端口号

    // 注册信号处理函数，处理SIGINT信号（通常由Ctrl+C触发），在接收到该信号时执行resetHandler函数，进行服务器的清理和资源释放操作
    signal(SIGINT,resetHandler); 

    EventLoop loop; // 创建事件循环对象
    InetAddress addr(ip,port); // 创建InetAddress对象，指定服务器的IP地址和端口号
    ChatServer server(&loop,addr,"ChatServer"); // 创建ChatServer对象，传入事件循环对象、InetAddress对象和服务器的名字
    server.start(); // 启动服务器，开始监听用户的连接请求
    loop.loop(); // 进入事件循环，等待和处理用户的连接和消息事件
    return 0;

} 