cmake_minimum_required(VERSION 3.24)
project(ov2)

add_executable(tiny-ov2
        tiny-ov2/ov2.c
        tiny-ov2/province_definitions.c tiny-ov2/province_definitions.h
        tiny-ov2/csv.c tiny-ov2/csv.h
        tiny-ov2/game_state.c tiny-ov2/game_state.h
        tiny-ov2/parse.c tiny-ov2/parse.h
        tiny-ov2/fs.c tiny-ov2/fs.h
        tiny-ov2/ui.c tiny-ov2/ui.h)
set_property(TARGET tiny-ov2 PROPERTY C_STANDARD 90)
target_link_libraries(tiny-ov2 SDL2 GL GLU SOIL)