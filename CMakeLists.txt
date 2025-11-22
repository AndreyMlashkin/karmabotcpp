cmake_minimum_required(VERSION 3.15)
project(karmabotcpp CXX)

find_package(tgbot REQUIRED CONFIG)
find_package(cpr REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(${PROJECT_NAME} main.cpp
    karmaloader.h karmaloader.cpp
    karmabot.h karmabot.cpp
    misc.h misc.cpp
    currenciesbroadcaster.h currenciesbroadcaster.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_link_libraries(${PROJECT_NAME} PRIVATE tgbot::tgbot cpr::cpr nlohmann_json::nlohmann_json)
