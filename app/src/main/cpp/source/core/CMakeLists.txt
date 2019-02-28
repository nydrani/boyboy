add_library(bboycore SHARED bboycore.cpp bboycore.hpp)

target_link_libraries(bboycore
        bboytools
        bboyshapes
        log
        EGL
        GLESv3
        android)
