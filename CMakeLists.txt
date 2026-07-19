cmake_minimum_required(VERSION 3.21)

project(ChatTest VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Network)

qt_standard_project_setup()

qt_add_executable(ChatTest
    src/main.cpp
    src/ChatWindow.h
    src/ChatWindow.cpp
    src/LoginWindow.h
    src/LoginWindow.cpp
)

qt_add_executable(ChatServer
    src/server_main.cpp
    src/ServerDatabase.h
    src/ServerDatabase.cpp
)

target_link_libraries(ChatTest
    PRIVATE
        Qt6::Widgets
        Qt6::Network
)

target_link_libraries(ChatServer
    PRIVATE
        Qt6::Core
        Qt6::Sql
        Qt6::Network
)
