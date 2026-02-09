if(NOT MYCHAT_BUILD_STATUSSERVER)
  return()
endif()

add_executable(StatusServer
  AsioIOServicePool.cpp
  ConfigMgr.cpp
  DistLock.cpp
  MysqlDao.cpp
  MysqlMgr.cpp
  RedisMgr.cpp
  StatusServer.cpp
  StatusServiceImpl.cpp
  message.grpc.pb.cc
  message.pb.cc
)

target_include_directories(StatusServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(StatusServer PRIVATE mychat_deps)
