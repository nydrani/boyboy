add_library(bboycore SHARED bboycore.cpp)

target_link_libraries(bboycore
        bboytools
        bboyshapes
        log
        EGL
        GLESv3
        android)
