cmake_minimum_required(VERSION 3.15)
project(karmabotcpp CXX)

find_package(tgbot REQUIRED CONFIG)
find_package(cpr REQUIRED)
find_package(nlohmann_json REQUIRED)

set(GIT_VERSION_CPP "${CMAKE_BINARY_DIR}/generated/git_version.cpp")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")

add_custom_target(generate_git_version_cpp ALL
    COMMAND ${CMAKE_COMMAND}
        -DOUTPUT_FILE=${GIT_VERSION_CPP}
        -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_SOURCE_DIR}/cmake/GenerateGitVersion.cmake
    BYPRODUCTS ${GIT_VERSION_CPP}
    COMMENT "Generating git version source"
    VERBATIM)

add_executable(${PROJECT_NAME} main.cpp
    ${GIT_VERSION_CPP}
    git_version.h
    karmaloader.h karmaloader.cpp
    karmabot.h karmabot.cpp
    misc.h misc.cpp
    currenciesbroadcaster.h currenciesbroadcaster.cpp)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
set_source_files_properties(${GIT_VERSION_CPP} PROPERTIES GENERATED TRUE)
add_dependencies(${PROJECT_NAME} generate_git_version_cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE tgbot::tgbot cpr::cpr nlohmann_json::nlohmann_json)
