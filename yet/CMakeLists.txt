file(GLOB YET_SRC "*.cc")
file(GLOB YET_RTMP_SRC "yet_rtmp/*.cc")
add_executable(yet ${YET_SRC} ${YET_RTMP_SRC})
target_link_libraries(yet ${COMMON_LIBS})

