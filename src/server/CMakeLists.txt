
aux_source_directory(. SRC) # 将当前目录下的所有源文件添加到变量SRC中
aux_source_directory(./db DB_SRC) # 将db目录下的所有源文件添加到变量DB_SRC中
aux_source_directory(./model MODEL_SRC) # 将model目录下的所有源文件添加到变量MODEL_SRC中
aux_source_directory(./redis REDIS_SRC) # 将model目录下的所有源文件添加到变量MODEL_SRC中

add_executable(chatserver ${SRC} ${DB_SRC} ${MODEL_SRC} ${REDIS_SRC}) # 生成可执行文件
target_link_libraries(chatserver muduo_net muduo_base pthread mysqlclient hiredis) # 链接库