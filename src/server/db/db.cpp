#include"db.h"
#include<muduo/base/Logging.h> // 包含muduo库中的日志记录功能，提供日志输出和管理功能。

static string server="127.0.0.1";
static string user="root";
static string password="160159";
static string dbname="chat";


MySQL::MySQL()
{
    _conn=mysql_init(nullptr); // 初始化MySQL连接对象，返回一个MYSQL结构体指针。
}

MySQL::~MySQL()
{
    if(_conn!=nullptr)
    {
        mysql_close(_conn); // 关闭MySQL连接，释放资源。
    }
}

//连接数据库
bool MySQL::connect()
{
    MYSQL *p=mysql_real_connect(_conn,server.c_str(),user.c_str(),password.c_str(),dbname.c_str(),3306,nullptr,0); // 连接到MySQL数据库，返回一个MYSQL结构体指针。
    if(p!=nullptr)
    {
        mysql_query(_conn,"set names utf8"); // 设置MySQL连接的字符集为UTF-8。
        LOG_INFO<<"连接数据库成功!"; // 输出日志信息，表示连接数据库成功。
    }else 
    {
        LOG_INFO<<"连接数据库失败!"; // 输出日志信息，表示连接数据库失败。
    }
    return p;
}

//更新操作
bool MySQL::update(string sql)
{
    if(mysql_query(_conn,sql.c_str()))
    {
        // 输出日志信息，表示更新操作失败，并包含文件名、行号和SQL语句。
        LOG_INFO<<__FILE__<<":"<<__LINE__<<":"<<sql<<"更新失败! 错误信息: "<<mysql_error(_conn); 
        return false;
    }
    return true;
}

//查询操作
MYSQL_RES* MySQL::query(string sql)
{
    if(mysql_query(_conn,sql.c_str()))
    {
        LOG_INFO<<__FILE__<<":"<<__LINE__<<"："<<sql<<"查询失败!";
        return nullptr;
    }
    // 返回查询结果，使用mysql_use_result函数获取结果集。
    return mysql_use_result(_conn); 
}

//获取连接对象
MYSQL* MySQL::getConnection()
{
    return _conn; // 返回当前的MySQL连接对象。
}