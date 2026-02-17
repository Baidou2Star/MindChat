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
find_package(CURL REQUIRED)

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
  cmake_parse_arguments(MCS "" "ENTRY" "SOURCES;EXCLUDE_SOURCES" ${ARGN})
  if(NOT MCS_ENTRY)
    message(FATAL_ERROR "mychat_add_server(${target_name}) requires ENTRY")
  endif()

  if(NOT MCS_SOURCES)
    file(GLOB MCS_SOURCES CONFIGURE_DEPENDS
      RELATIVE "${CMAKE_SOURCE_DIR}"
      "${CMAKE_SOURCE_DIR}/${source_dir}/*.cpp"
      "${CMAKE_SOURCE_DIR}/${source_dir}/*.cc"
    )
    list(REMOVE_ITEM MCS_SOURCES "${source_dir}/${MCS_ENTRY}")
    if(MCS_EXCLUDE_SOURCES)
      list(REMOVE_ITEM MCS_SOURCES ${MCS_EXCLUDE_SOURCES})
    endif()
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
)

mychat_add_server(ChatServer2 ChatServer2
  ENTRY ChatServer.cpp
)

mychat_add_server(GateServerWin GateServerWin
  ENTRY GateServerWin.cpp
  EXCLUDE_SOURCES
    GateServerWin/main.cpp
)

mychat_add_server(StatusServer StatusServer
  ENTRY StatusServer.cpp
)

mychat_add_server(ResourceServer ResourceServer
  ENTRY ResourceServer.cpp
)

mychat_add_server(LLMServer LLMServer
  ENTRY LLMServer.cpp
)

target_link_libraries(LLMServer PRIVATE CURL::libcurl)
