file(GLOB SRC "*.cc")
add_executable(yet ${SRC})
target_link_libraries(yet ${COMMON_LIBS})

