cmake_minimum_required(VERSION 3.20)

project(myChatServers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MYCHAT_BUILD_CHATSERVER "Build ChatServer" ON)
option(MYCHAT_BUILD_CHATSERVER2 "Build ChatServer2" ON)
option(MYCHAT_BUILD_GATESERVER "Build GateServerWin" ON)
option(MYCHAT_BUILD_STATUSSERVER "Build StatusServer" ON)
option(MYCHAT_BUILD_RESOURCESERVER "Build ResourceServer" ON)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem system)
find_package(jsoncpp CONFIG REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(mysql-connector-cpp CONFIG REQUIRED)

add_library(mychat_deps INTERFACE)

if(TARGET mysqlcppconn8)
  set(MYCHAT_MYSQL_TARGET mysqlcppconn8)
elseif(TARGET mysqlcppconn)
  set(MYCHAT_MYSQL_TARGET mysqlcppconn)
else()
  message(FATAL_ERROR "mysql-connector-cpp target not found (expected mysqlcppconn8 or mysqlcppconn)")
endif()

target_link_libraries(mychat_deps INTERFACE
  Threads::Threads
  Boost::filesystem
  Boost::system
  jsoncpp_lib
  hiredis::hiredis
  protobuf::libprotobuf
  gRPC::grpc++
  ${MYCHAT_MYSQL_TARGET}
)

add_subdirectory(ChatServer)
add_subdirectory(ChatServer2)
add_subdirectory(GateServerWin)
add_subdirectory(StatusServer)
add_subdirectory(ResourceServer)
