cmake_minimum_required (VERSION 3.3)
# 设置C/C++标准
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
# 声明一个项目
project(BlueRoseRender)
# 我们使用Unicode字符编码
add_definitions(-D "UNICODE")
add_definitions(-D "_UNICODE")
# 获取程序的源文件和头文件
file(GLOB_RECURSE src source/*.cxx source/*.c source/*.cc source/*.cpp)
file(GLOB_RECURSE headers source/*.hpp source/*.h source/*.hh)
file(GLOB_RECURSE headers include/*.hpp include/*.h include/*.hh)
# 添加程序目标
add_executable(BlueRoseRender ${src} ${headers})
# 定义目标的包含路径
target_include_directories(BlueRoseRender PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# 将目标链接到windows的一些API上
set(PLATFORM_FRAMEWORKS psapi user32 advapi32 iphlpapi userenv ws2_32)
target_link_libraries(BlueRoseRender PRIVATE ${PLATFORM_FRAMEWORKS})
# 给x86平台添加SSE2指令集
if(CMAKE_SIZEOF_VOID_P EQUAL 4) # 32-bit
    SET(CMAKE_CXX_FLAGS "/arch:SSE2")
endif()
# 添加目录结构
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${src})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${headers})