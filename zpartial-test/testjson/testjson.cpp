// 包含json
#include<nlohmann/json.hpp>
using json=nlohmann::json;
#include<iostream>
#include<vector>
#include<map>
#include<string>
using namespace std;

// json序列化
string func1()
{
    json js;
    js["msg_type"]=2;
    js["from"]="zhangsan";
    js["to"]="li si";
    js["msg"]="hello,li si";
    string sendBuf=js.dump(); // 将json对象转换成字符串
    return sendBuf;
}
string func2()
{
    json js;
    js["id"]={1,2,3,4,5}; // json对象中可以存储数组
    js["name"]={"zhangsan","lisi","wangwu"}; // json对象中可以存储数组
    js["msg"]["zhangsan"]="hello,li si"; // json对象中可以存储对象
    js["msg"]["lisi"]="hello,zhangsan"; // json对象中可以存储对象
    js["msg"]={{"zhangsan","hello,li si"},{"lisi","hello,zhangsan"}}; // json对象中可以存储数组对象
    return js.dump();
}

string func3()
{
    json js;
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    js["list"]=vec; // json对象中可以存储数组

    map<int,string> m;
    m.insert({1,"zhangsan"});
    m.insert({2,"lisi"});
    m.insert({3,"wangwu"});
    js["path"]=m;
    string buf=js.dump(); // 将json对象转换成字符串
    return buf;
}

int main()
{
    string recvbuf=func3();
    //json的反序列化
    json buf=json::parse(recvbuf);
    // cout<<buf["msg_type"]<<endl;
    // cout<<buf["from"]<<endl;
    // cout<<buf["to"]<<endl;
    // cout<<buf["msg"]<<endl;

    // cout<<buf["id"][2]<<endl;
    
    vector<int> vec=buf["list"];
    for(auto &e:vec)
    {
        cout<<e<<" ";
    }
    cout<<endl;

    map<int,string> m=buf["path"];
    for(auto &e:m)
    {
        cout<<e.first<<" "<<e.second<<endl;
    }   


    // func1();
    // func2();
    // func3();
    return 0;
} //g++ testjson.cpp -o 111
