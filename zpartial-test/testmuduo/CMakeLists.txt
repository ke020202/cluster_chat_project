cmake_minimum_required(VERSION 3.10)
project(testmuduo)

#配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#设置可执行文件最终存储的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 添加源文件
aux_source_directory(${PROJECT_SOURCE_DIR} src)

# 生成可执行文件
add_executable(111 ${src})

# 链接库文件
target_link_libraries(111 muduo_net muduo_base pthread) 