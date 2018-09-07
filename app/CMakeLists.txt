cmake_minimum_required(VERSION 3.4.1)

add_subdirectory(src/main/cpp/glm)

add_library(bboycore
            SHARED
            src/main/cpp/bboycore.cpp)

target_link_libraries(bboycore
                      GLESv3
                      log
                      EGL
                      glm_static
                      android)
