cmake_minimum_required(VERSION 3.20)
project(cm0102-mac C)

set(CMAKE_C_STANDARD 11)

find_package(SDL2 REQUIRED)

add_executable(cm0102
    src/main.c
    src/demo.c
    src/si/display.c
    src/si/os_compat.c
    src/si/sound.c
    src/si/input.c
    src/si/rgn_loader.c
    src/si/gui_utils.c
    src/si/gui_glyphs.c
    src/si/gui_font.c
    src/si/gui_text.c
    src/si/guio.c
    src/si/area.c
    src/si/scrman.c
    src/si/screen_stack.c
    src/si/scrman_input.c
    src/si/langlib.c
    src/si/gui_menu.c
    src/cm3/database.c
)

target_include_directories(cm0102 PRIVATE include ${SDL2_INCLUDE_DIRS})
target_link_libraries(cm0102 PRIVATE ${SDL2_LIBRARIES})

# Runs functions of the port for the differential test in tests/diff.
add_executable(port_driver
    tests/diff/port_driver.c
    tests/diff/port_scene.c
    tests/diff/port_stack.c
    tests/diff/port_helpers.c
    src/si/display.c
    src/si/rgn_loader.c
    src/si/gui_utils.c
    src/si/gui_glyphs.c
    src/si/gui_font.c
    src/si/gui_text.c
    src/si/os_compat.c
    src/si/guio.c
    src/si/area.c
    src/si/scrman.c
    src/si/screen_stack.c
    src/si/scrman_input.c
    src/si/langlib.c
    src/si/gui_menu.c
)
target_include_directories(port_driver PRIVATE include ${SDL2_INCLUDE_DIRS})
target_link_libraries(port_driver PRIVATE ${SDL2_LIBRARIES})

if(APPLE)
    set_target_properties(cm0102 PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Championship Manager 01/02"
        MACOSX_BUNDLE_GUI_IDENTIFIER "uk.co.sigames.cm0102"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )
endif()
