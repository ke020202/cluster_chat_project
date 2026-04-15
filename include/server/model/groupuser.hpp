#pragma once

#include"user.hpp"

class GroupUser:public User
{
public:
    void setRole(string role) {this->role=role;}
    string getRole() {return this->role;}
private:
    string role; // 群成员的角色，例如群主、管理员、普通成员等
};