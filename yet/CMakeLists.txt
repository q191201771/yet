file(GLOB YET_SRC "*.cc")
file(GLOB YET_COMMON_SRC "yet_common/*.cc")
file(GLOB YET_HTTP_FLV_SRC "yet_http_flv/*.cc")
file(GLOB YET_RTMP_SRC "yet_rtmp/*.cc")
add_executable(yet ${YET_SRC} ${YET_COMMON_SRC} ${YET_HTTP_FLV_SRC} ${YET_RTMP_SRC})
target_link_libraries(yet ${COMMON_LIBS})

