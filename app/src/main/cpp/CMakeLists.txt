file(GLOB_RECURSE SHAPES_SOURCE shapes/*.cpp)
file(GLOB_RECURSE SHAPES_HEADER shapes/*.hpp)

file(GLOB_RECURSE TOOLS_SOURCE tools/*.cpp)
file(GLOB_RECURSE TOOLS_HEADER tools/*.hpp)

add_subdirectory(glm)

add_library(bboyutils STATIC ${TOOLS_SOURCE} ${TOOLS_HEADER} ${SHAPES_SOURCE} ${SHAPES_HEADER})

target_link_libraries(bboyutils
                      GLESv3)