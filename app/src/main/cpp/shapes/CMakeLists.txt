file(GLOB SHAPES_SOURCE *.cpp)
file(GLOB SHAPES_HEADER *.hpp)

add_library(bboyshapes SHARED ${SHAPES_SOURCE} ${SHAPES_HEADER})