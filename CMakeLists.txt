cmake_minimum_required(VERSION 3.22)
project(crime_life)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(thirdparty/stduuid)

add_executable(crime_life src/main.cpp)
target_link_libraries(crime_life stduuid)
