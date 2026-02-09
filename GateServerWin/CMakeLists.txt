if(NOT MYCHAT_BUILD_GATESERVER)
  return()
endif()

add_executable(GateServerWin
  AsioIOServicePool.cpp
  ConfigMgr.cpp
  CServer.cpp
  DistLock.cpp
  GateServerWin.cpp
  HttpConnection.cpp
  LogicSystem.cpp
  MysqlDao.cpp
  MysqlMgr.cpp
  RedisMgr.cpp
  StatusGrpcClient.cpp
  VerifyGrpcClient.cpp
  message.grpc.pb.cc
  message.pb.cc
)

target_include_directories(GateServerWin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(GateServerWin PRIVATE mychat_deps)
