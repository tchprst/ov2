cmake_minimum_required(VERSION 3.23)
project(ov2)

add_executable(ov2
        src/ov2.c
        src/province_definitions.c src/province_definitions.h
        src/csv.c src/csv.h
        src/game_state.c src/game_state.h
        src/parse.c src/parse.h
        src/fs.c src/fs.h
        src/ui.c src/ui.h
        src/ui_event.c src/ui_event.h
        src/game_tick.c src/game_tick.h
        src/bitmap_font.c src/bitmap_font.h)
set_property(TARGET ov2 PROPERTY C_STANDARD 90)
target_link_libraries(ov2 SDL2 SDL2_ttf GL GLU SOIL)