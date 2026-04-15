#pragma once

#include<mysql/mysql.h> // 包含MySQL C API的头文件，提供与MySQL数据库交互的函数和数据结构。
#include<string> // 包含C++标准库中的字符串类，提供字符串操作功能。
using namespace std; // 使用C++标准库的命名空间，避免在代码中频繁使用std::前缀。

class MySQL
{
public:
    MySQL(); //构造函数，用于初始化MySQL对象。
    ~MySQL(); //析构函数，用于释放MySQL对象占用的资源
    bool connect(); //连接数据库的方法，返回一个布尔值表示连接是否成功
    bool update(string sql);
    //执行查询操作的方法，接受一个SQL语句作为参数，并返回一个MYSQL_RES指针，指向查询结果。
    MYSQL_RES* query(string sql); 
    //获取连接对象的方法，返回一个MYSQL指针，指向当前的数据库连接。
    
    MYSQL* getConnection();

private:
    MYSQL *_conn; //MYSQL结构体指针，用于表示与MySQL数据库的连接。
};