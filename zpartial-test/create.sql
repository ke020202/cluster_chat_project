-- 1. 用户表（仅修正注释语法）
create table User(
    id int primary key auto_increment comment '用户ID',
    name varchar(50) not null unique comment '用户名',
    password varchar(50) not null comment '密码',
    state enum('online','offline') default 'offline' comment '当前登录状态'
)COMMENT='用户表';

-- 2. 好友表（仅修正注释语法）
create table Friend(
    userid int not null comment '用户ID',
    friendid int not null comment '好友ID',
    primary key(userid,friendid)
)comment '好友表';

-- 3. 群组表（仅修正注释语法）
create table ALLGroup(
    id int primary key auto_increment comment '组id',
    groupname varchar(50) not null comment '组名称',
    groupdesc varchar(200) default '' comment '组描述'
)comment '群组表';

-- 4. 组员表（100%还原你的主键：groupid 单独主键，无任何修改）
create table GroupUser(
    groupid int not null comment '组id',
    userid int not null comment '组员id',
    grouprole enum('creator','normal') default 'normal' comment '组内角色'
)comment '组员表';

-- 5. 离线消息表（100%还原你的主键：userid 单独主键）
create table OfflineMessage(
    userid int not null comment '用户id',
    message varchar(500) not null comment '离线消息(存储Json字符串)'
)comment '离线消息表';