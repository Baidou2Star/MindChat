if(NOT MYCHAT_BUILD_CHATSERVER)
  return()
endif()

add_executable(ChatServer2
  AsioIOServicePool.cpp
  ChatGrpcClient.cpp
  ChatServer.cpp
  ChatServiceImpl.cpp
  ConfigMgr.cpp
  CServer.cpp
  CSession.cpp
  DistLock.cpp
  LogicSystem.cpp
  MsgNode.cpp
  MysqlDao.cpp
  MysqlMgr.cpp
  RedisMgr.cpp
  StatusGrpcClient.cpp
  UserMgr.cpp
  utils.cpp
  message.grpc.pb.cc
  message.pb.cc
)

target_include_directories(ChatServer2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ChatServer2 PRIVATE mychat_deps)
