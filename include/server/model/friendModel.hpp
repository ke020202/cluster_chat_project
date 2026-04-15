#pragma once

#include<vector>
#include"user.hpp"
using namespace std;


// 维护好友信息的数据模型类，提供对好友数据的增删改查功能
class FriendModel
{
public:
    // 添加好友关系，参数为用户ID和好友ID
    void insert(int userid,int friendid);
    // 查询用户的好友列表，参数为用户ID，返回一个包含好友ID的
    vector<User> query(int userid);
};