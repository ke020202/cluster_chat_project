aux_source_directory(. SRC)
add_executable(client ${SRC})

target_link_libraries(client pthread)