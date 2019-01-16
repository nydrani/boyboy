file(GLOB SHAPES_SOURCE *.cpp)
file(GLOB SHAPES_HEADER *.hpp)

add_library(bboyshapes ${SHAPES_SOURCE} ${SHAPES_HEADER})

target_link_libraries(bboyshapes PUBLIC
        bboytools
        GLESv3)
