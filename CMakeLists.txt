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
    src/si/progress_box.c
    src/si/utils.c
    src/si/date.c
    src/si/random.c
    src/cm3/background.c
    src/cm3/menubar.c
    src/cm3/file_screens.c
    src/cm3/setup.c
    src/cm3/database.c
    src/cm3/database_links.c
    src/cm3/key_index.c
    src/cm3/key_ids_table.c
    src/cm3/key_nation.c
)

target_include_directories(cm0102 PRIVATE include ${SDL2_INCLUDE_DIRS})
target_link_libraries(cm0102 PRIVATE ${SDL2_LIBRARIES})

# Runs functions of the port for the differential test in tests/diff.
add_executable(port_driver
    tests/diff/port_driver.c
    tests/diff/port_scene.c
    tests/diff/port_stack.c
    tests/diff/port_helpers.c
    tests/diff/port_start.c
    tests/diff/port_leagues.c
    tests/diff/port_progress.c
    tests/diff/port_scroll.c
    tests/diff/port_select.c
    tests/diff/port_dates.c
    tests/diff/port_entry.c
    tests/diff/port_database.c
    src/si/display.c
    src/si/rgn_loader.c
    src/si/gui_utils.c
    src/si/gui_glyphs.c
    src/si/gui_font.c
    src/si/gui_text.c
    src/si/os_compat.c
    src/si/guio.c
    src/si/input.c
    src/si/area.c
    src/si/scrman.c
    src/si/screen_stack.c
    src/si/scrman_input.c
    src/si/langlib.c
    src/si/gui_menu.c
    src/si/progress_box.c
    src/si/utils.c
    src/si/date.c
    src/si/random.c
    src/cm3/background.c
    src/cm3/menubar.c
    src/cm3/file_screens.c
    src/cm3/setup.c
    src/cm3/database.c
    src/cm3/database_links.c
    src/cm3/key_index.c
    src/cm3/key_ids_table.c
    src/cm3/key_nation.c
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

# The recompiled original (docs/specs/2026-10-14-recompile-spec.md). tools/recomp translates the
# player's own cm0102.exe into C in the build folder; that C is a build product and stays out of git.
# By default the first cm0102.exe in a folder above the repo (a git worktree sits deeper).
set(CM0102_EXE_DEFAULT "${CMAKE_SOURCE_DIR}/../cm0102.exe")
set(dir "${CMAKE_SOURCE_DIR}")
foreach(level RANGE 6)
    get_filename_component(dir "${dir}" DIRECTORY)
    if(EXISTS "${dir}/cm0102.exe")
        set(CM0102_EXE_DEFAULT "${dir}/cm0102.exe")
        break()
    endif()
endforeach()
set(CM0102_EXE "${CM0102_EXE_DEFAULT}" CACHE FILEPATH "The player's cm0102.exe")
if(EXISTS "$ENV{HOME}/.venvs/cm0102/bin/python")
    set(RECOMP_PYTHON_DEFAULT "$ENV{HOME}/.venvs/cm0102/bin/python")
else()
    set(RECOMP_PYTHON_DEFAULT "python3")
endif()
set(RECOMP_PYTHON "${RECOMP_PYTHON_DEFAULT}" CACHE FILEPATH "A Python with capstone, for tools/recomp")

if(EXISTS "${CM0102_EXE}")
    set(RECOMP_SHARDS 128)
    set(RECOMP_OUT "${CMAKE_BINARY_DIR}/recomp")
    set(RECOMP_GEN "${RECOMP_OUT}/gen/table.c" "${RECOMP_OUT}/gen/functions.h")
    math(EXPR RECOMP_LAST "${RECOMP_SHARDS} - 1")
    foreach(i RANGE ${RECOMP_LAST})
        string(LENGTH "${i}" n)
        if(n EQUAL 1)
            set(i "00${i}")
        elseif(n EQUAL 2)
            set(i "0${i}")
        endif()
        list(APPEND RECOMP_GEN "${RECOMP_OUT}/gen/fn_${i}.c")
    endforeach()
    file(GLOB RECOMP_WIN CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/recomp_rt/win/*.c")
    file(GLOB RECOMP_TOOL CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tools/recomp/*.py")
    set(RECOMP_RUNTIME_ARGS "")
    foreach(f ${RECOMP_WIN})
        list(APPEND RECOMP_RUNTIME_ARGS --runtime "${f}")
    endforeach()
    add_custom_command(
        OUTPUT "${RECOMP_OUT}/recomp.stamp"
        BYPRODUCTS ${RECOMP_GEN}
        COMMAND "${RECOMP_PYTHON}" -I "${CMAKE_SOURCE_DIR}/tools/recomp/recomp.py"
            --exe "${CM0102_EXE}" --out "${RECOMP_OUT}"
            --functions "${CMAKE_SOURCE_DIR}/tools/recomp/functions.txt" --all --entries 0x947a13
            --shards ${RECOMP_SHARDS} ${RECOMP_RUNTIME_ARGS} --stamp "${RECOMP_OUT}/recomp.stamp"
        DEPENDS ${RECOMP_TOOL} "${CMAKE_SOURCE_DIR}/tools/recomp/functions.txt" ${RECOMP_WIN}
            "${CMAKE_SOURCE_DIR}/src/recomp_rt/rt.c" "${CMAKE_SOURCE_DIR}/src/recomp_rt/heap.c"
        COMMENT "Recompiling cm0102.exe to C"
        VERBATIM)
    add_custom_target(recomp_gen DEPENDS "${RECOMP_OUT}/recomp.stamp")

    add_executable(cm0102_recomp ${RECOMP_GEN}
        src/recomp_rt/rt.c
        src/recomp_rt/heap.c
        ${RECOMP_WIN}
    )
    add_dependencies(cm0102_recomp recomp_gen)
    target_include_directories(cm0102_recomp PRIVATE src/recomp_rt src/recomp_rt/win "${RECOMP_OUT}/gen"
        ${SDL2_INCLUDE_DIRS})
    target_compile_definitions(cm0102_recomp PRIVATE CM0102_DEFAULT_EXE="${CM0102_EXE}")
    # The generated C: exact integer wrap, no strict aliasing, no warnings (it is not read by people).
    set_source_files_properties(${RECOMP_GEN} PROPERTIES GENERATED TRUE COMPILE_OPTIONS "-O1;-w")
    target_compile_options(cm0102_recomp PRIVATE -fno-strict-aliasing -fwrapv -O1)
    # Every call of the original is a host call: give the main thread a deep stack.
    target_link_options(cm0102_recomp PRIVATE "LINKER:-stack_size,0x10000000")
    target_link_libraries(cm0102_recomp PRIVATE ${SDL2_LIBRARIES} m)
else()
    message(STATUS "cm0102_recomp: ${CM0102_EXE} not found, so the recompiled game is not built")
endif()
