cmake_minimum_required(VERSION 3.20)
project(myChatServers LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED COMPONENTS filesystem system uuid)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Threads REQUIRED)
find_package(hiredis CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(unofficial-mysql-connector-cpp CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)

function(_pick_first_existing_target out_var)
  foreach(candidate IN LISTS ARGN)
    if(TARGET ${candidate})
      set(${out_var} ${candidate} PARENT_SCOPE)
      return()
    endif()
  endforeach()
  set(${out_var} "" PARENT_SCOPE)
endfunction()

_pick_first_existing_target(MYCHAT_PROTOBUF_TARGET
  protobuf::libprotobuf
  protobuf::protobuf
)
if(NOT MYCHAT_PROTOBUF_TARGET)
  message(FATAL_ERROR "Cannot find protobuf imported target.")
endif()

_pick_first_existing_target(MYCHAT_GRPCPP_TARGET
  gRPC::grpc++
)
if(NOT MYCHAT_GRPCPP_TARGET)
  message(FATAL_ERROR "Cannot find gRPC imported target gRPC::grpc++.")
endif()

_pick_first_existing_target(MYCHAT_HIREDIS_TARGET
  hiredis::hiredis
  hiredis::hiredis_static
  hiredis
)
if(NOT MYCHAT_HIREDIS_TARGET)
  message(FATAL_ERROR "Cannot find hiredis imported target.")
endif()

_pick_first_existing_target(MYCHAT_JSONCPP_TARGET
  JsonCpp::JsonCpp
  jsoncpp_lib
)
if(NOT MYCHAT_JSONCPP_TARGET)
  message(FATAL_ERROR "Cannot find jsoncpp imported target.")
endif()

_pick_first_existing_target(MYCHAT_MYSQLCPP_TARGET
  unofficial::mysql-connector-cpp::connector
  unofficial::mysql-connector-cpp::jdbc
)
if(NOT MYCHAT_MYSQLCPP_TARGET)
  message(FATAL_ERROR "Cannot find mysql-connector-cpp imported target.")
endif()

set(MYCHAT_COMMON_LIBS
  Threads::Threads
  Boost::filesystem
  Boost::system
  Boost::uuid
  ${MYCHAT_PROTOBUF_TARGET}
  ${MYCHAT_GRPCPP_TARGET}
  ${MYCHAT_HIREDIS_TARGET}
  ${MYCHAT_JSONCPP_TARGET}
  ${MYCHAT_MYSQLCPP_TARGET}
  lz4::lz4
  zstd::libzstd
  resolv
  dl
)

function(mychat_add_server target_name source_dir)
  cmake_parse_arguments(MCS "" "ENTRY" "SOURCES" ${ARGN})
  if(NOT MCS_ENTRY)
    message(FATAL_ERROR "mychat_add_server(${target_name}) requires ENTRY")
  endif()

  add_executable(${target_name}
    ${source_dir}/${MCS_ENTRY}
    ${MCS_SOURCES}
  )

  set_target_properties(${target_name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${target_name}"
  )

  target_compile_definitions(${target_name} PRIVATE BOOST_BIND_GLOBAL_PLACEHOLDERS)
  target_include_directories(${target_name} PRIVATE ${source_dir})
  target_link_libraries(${target_name} PRIVATE ${MYCHAT_COMMON_LIBS})

  if(EXISTS "${CMAKE_SOURCE_DIR}/${source_dir}/config.ini")
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${CMAKE_SOURCE_DIR}/${source_dir}/config.ini"
      "$<TARGET_FILE_DIR:${target_name}>/config.ini"
    )
  endif()
endfunction()

mychat_add_server(ChatServer ChatServer
  ENTRY ChatServer.cpp
  SOURCES
    ChatServer/AsioIOServicePool.cpp
    ChatServer/ChatGrpcClient.cpp
    ChatServer/ChatServiceImpl.cpp
    ChatServer/ConfigMgr.cpp
    ChatServer/CServer.cpp
    ChatServer/CSession.cpp
    ChatServer/DistLock.cpp
    ChatServer/LogicSystem.cpp
    ChatServer/message.grpc.pb.cc
    ChatServer/message.pb.cc
    ChatServer/MsgNode.cpp
    ChatServer/MysqlDao.cpp
    ChatServer/MysqlMgr.cpp
    ChatServer/RedisMgr.cpp
    ChatServer/StatusGrpcClient.cpp
    ChatServer/UserMgr.cpp
    ChatServer/utils.cpp
)

mychat_add_server(ChatServer2 ChatServer2
  ENTRY ChatServer.cpp
  SOURCES
    ChatServer2/AsioIOServicePool.cpp
    ChatServer2/ChatGrpcClient.cpp
    ChatServer2/ChatServiceImpl.cpp
    ChatServer2/ConfigMgr.cpp
    ChatServer2/CServer.cpp
    ChatServer2/CSession.cpp
    ChatServer2/DistLock.cpp
    ChatServer2/LogicSystem.cpp
    ChatServer2/message.grpc.pb.cc
    ChatServer2/message.pb.cc
    ChatServer2/MsgNode.cpp
    ChatServer2/MysqlDao.cpp
    ChatServer2/MysqlMgr.cpp
    ChatServer2/RedisMgr.cpp
    ChatServer2/StatusGrpcClient.cpp
    ChatServer2/UserMgr.cpp
    ChatServer2/utils.cpp
)

mychat_add_server(GateServerWin GateServerWin
  ENTRY main.cpp
  SOURCES
    GateServerWin/AsioIOServicePool.cpp
    GateServerWin/ConfigMgr.cpp
    GateServerWin/CServer.cpp
    GateServerWin/DistLock.cpp
    GateServerWin/HttpConnection.cpp
    GateServerWin/LogicSystem.cpp
    GateServerWin/message.grpc.pb.cc
    GateServerWin/message.pb.cc
    GateServerWin/MysqlDao.cpp
    GateServerWin/MysqlMgr.cpp
    GateServerWin/RedisMgr.cpp
    GateServerWin/StatusGrpcClient.cpp
    GateServerWin/VerifyGrpcClient.cpp
)

mychat_add_server(StatusServer StatusServer
  ENTRY StatusServer.cpp
  SOURCES
    StatusServer/AsioIOServicePool.cpp
    StatusServer/ConfigMgr.cpp
    StatusServer/DistLock.cpp
    StatusServer/message.grpc.pb.cc
    StatusServer/message.pb.cc
    StatusServer/MysqlDao.cpp
    StatusServer/MysqlMgr.cpp
    StatusServer/RedisMgr.cpp
    StatusServer/StatusServiceImpl.cpp
)

mychat_add_server(ResourceServer ResourceServer
  ENTRY ResourceServer.cpp
  SOURCES
    ResourceServer/AsioIOServicePool.cpp
    ResourceServer/base64.cpp
    ResourceServer/ConfigMgr.cpp
    ResourceServer/CServer.cpp
    ResourceServer/CSession.cpp
    ResourceServer/DistLock.cpp
    ResourceServer/FileInfo.cpp
    ResourceServer/FileSystem.cpp
    ResourceServer/FileWorker.cpp
    ResourceServer/LogicSystem.cpp
    ResourceServer/LogicWorker.cpp
    ResourceServer/message.grpc.pb.cc
    ResourceServer/message.pb.cc
    ResourceServer/MsgNode.cpp
    ResourceServer/MysqlDao.cpp
    ResourceServer/MysqlMgr.cpp
    ResourceServer/RedisMgr.cpp
    ResourceServer/UserMgr.cpp
)
