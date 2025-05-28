cmake_minimum_required(VERSION 3.16)
project(WarehouseServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

find_package(Boost REQUIRED COMPONENTS system)
find_package(PostgreSQL REQUIRED)
find_package(Threads REQUIRED)
find_package(fmt REQUIRED)

find_package(nlohmann_json 3.2.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PostgreSQL_INCLUDE_DIRS})

set(SOURCES
    main.cpp
    Lobby.cpp
    Logger.cpp
    SqlCommander.cpp
    BufferPool.cpp
)

set(HEADERS
    Lobby.h
    Logger.h
    SqlCommander.h
    BufferPool.h
)

add_executable(warehouse_server ${SOURCES} ${HEADERS})

target_link_libraries(warehouse_server 
    ${Boost_LIBRARIES}
    ${PostgreSQL_LIBRARIES}
    Threads::Threads
    fmt::fmt
    nlohmann_json::nlohmann_json
)

configure_file(${CMAKE_SOURCE_DIR}/server/src/config.env ${CMAKE_BINARY_DIR}/config.env COPYONLY)

install(TARGETS warehouse_server DESTINATION bin)