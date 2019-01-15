file(GLOB TOOLS_SOURCE *.cpp)
file(GLOB TOOLS_HEADER *.hpp)

add_library(bboytools ${TOOLS_SOURCE} ${TOOLS_HEADER})
