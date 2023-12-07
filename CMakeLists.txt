# 设置最低 CMake 版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(HPC-WebServer)

# 查找线程库
find_package(Threads REQUIRED)

# 设置语言标准编译
set(CMAKE_CXX_STANDARD 11)

# 添加库文件
include_directories(${CMAKE_SOURCE_DIR}/include)

# 添加源文件
aux_source_directory(${CMAKE_SOURCE_DIR}/src CPP_PATH)

# 设置可执行文件生成目录
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# 添加可执行文件
add_executable(myserver ${CPP_PATH})

# 将线程库链接到可执行文件
target_link_libraries(myserver ${CMAKE_THREAD_LIBS_INIT})
# 链接MySQL库
target_link_libraries(myserver mysqlclient)

