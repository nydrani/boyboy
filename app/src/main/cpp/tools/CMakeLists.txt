file(GLOB TOOLS_SOURCE *.cpp)
file(GLOB TOOLS_HEADER *.hpp)

add_library(bboytools SHARED ${TOOLS_SOURCE} ${TOOLS_HEADER})