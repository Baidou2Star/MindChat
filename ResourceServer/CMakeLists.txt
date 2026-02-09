if(NOT MYCHAT_BUILD_RESOURCESERVER)
  return()
endif()

add_executable(ResourceServer
  AsioIOServicePool.cpp
  base64.cpp
  ConfigMgr.cpp
  CServer.cpp
  CSession.cpp
  DistLock.cpp
  FileInfo.cpp
  FileSystem.cpp
  FileWorker.cpp
  LogicSystem.cpp
  LogicWorker.cpp
  MsgNode.cpp
  MysqlDao.cpp
  MysqlMgr.cpp
  RedisMgr.cpp
  ResourceServer.cpp
  UserMgr.cpp
  message.grpc.pb.cc
  message.pb.cc
)

target_include_directories(ResourceServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ResourceServer PRIVATE mychat_deps)
