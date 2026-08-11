cmake_minimum_required(VERSION 3.14)
project(miniweb CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(MINIWEB_BUILD_TESTS "Build unit tests" ON)
option(MINIWEB_COVERAGE "Enable gcov coverage instrumentation" OFF)

if(MINIWEB_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -g -O0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

include(FetchContent)

# ---- 四个 header-only 库（仅下载源码）----
FetchContent_Declare(cpp-httplib GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git GIT_TAG v0.15.3   GIT_SHALLOW TRUE)
FetchContent_Populate(cpp-httplib)
FetchContent_Declare(websocketpp  GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git   GIT_TAG 0.8.2     GIT_SHALLOW TRUE)
FetchContent_Populate(websocketpp)
FetchContent_Declare(asio         GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git    GIT_TAG asio-1-28-2 GIT_SHALLOW TRUE)
FetchContent_Populate(asio)

# RapidJSON：复用本机源树（全局 git insteadOf 把该 URL 重写为本地坏裸仓）。
set(RAPIDJSON_INCLUDE_DIR /home/li/peiking88/tdx-cpp/external/rapidjson/include
    CACHE PATH "rapidjson include 目录")
if(NOT EXISTS ${RAPIDJSON_INCLUDE_DIR}/rapidjson/document.h)
    message(FATAL_ERROR "rapidjson include 未找到: ${RAPIDJSON_INCLUDE_DIR}，用 -DRAPIDJSON_INCLUDE_DIR=... 指定")
endif()

find_package(OpenSSL REQUIRED)

# ---- 核心库（不含 main，供主程序与测试共用）----
add_library(miniweb_core STATIC
    src/JsonCodec.cpp
    src/HttpHandler.cpp
    src/WebSocketHandler.cpp
    src/ManagementServer.cpp
)
target_include_directories(miniweb_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${cpp-httplib_SOURCE_DIR}
    ${websocketpp_SOURCE_DIR}
    ${asio_SOURCE_DIR}/asio/include
    ${RAPIDJSON_INCLUDE_DIR}
)
target_link_libraries(miniweb_core PUBLIC OpenSSL::SSL OpenSSL::Crypto pthread)
target_compile_definitions(miniweb_core PUBLIC ASIO_STANDALONE CPPHTTPLIB_OPENSSL_SUPPORT NOMINMAX)
target_compile_options(miniweb_core PRIVATE -Wno-deprecated-declarations -Wno-unused-parameter)

# ---- 主可执行 ----
add_executable(miniweb src/main.cpp)
target_link_libraries(miniweb PRIVATE miniweb_core)

# ---- 单元测试 ----
if(MINIWEB_BUILD_TESTS)
    enable_testing()
    foreach(t json rate_limiter host server_failures)
        add_executable(test_${t} tests/test_${t}.cpp)
        target_link_libraries(test_${t} PRIVATE miniweb_core)
        target_include_directories(test_${t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests ${CMAKE_CURRENT_SOURCE_DIR}/src)
        add_test(NAME ${t} COMMAND test_${t} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endforeach()
endif()
