cmake_minimum_required(VERSION 3.4.1)

add_subdirectory(src/main/cpp)

add_library(bboycore SHARED src/main/cpp/bboycore.cpp)

target_link_libraries(bboycore
                      bboyutils
                      log
                      EGL
                      glm_static
                      android)
