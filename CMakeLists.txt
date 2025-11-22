cmake_minimum_required(VERSION 3.15)
project(karmabotcpp CXX)

find_package(tgbot REQUIRED CONFIG)

add_executable(${PROJECT_NAME} main.cpp
    karmaloader.h karmaloader.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_link_libraries(${PROJECT_NAME} tgbot::tgbot)
