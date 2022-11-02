cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

project(obb_editor CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "-Wall -Wformat")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

if(MSVC)
    SET(CMAKE_CXX_FLAGS "/EHsc")
    SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
endif()

if(NOT EMSCRIPTEN)
    find_package(SDL2 REQUIRED)
endif(NOT EMSCRIPTEN)

add_executable(obb_editor
    third_party/pugixml/pugixml.cpp
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_demo.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_sdl.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
    src/main.cpp
)

include_directories(
    ${SDL2_INCLUDE_DIRS}
    third_party
    third_party/imgui
    third_party/imgui/backends
)

if(EMSCRIPTEN)
    # WebAssembly
    set(CMAKE_EXECUTABLE_SUFFIX .html)

    target_compile_options(obb_editor PRIVATE
        -sUSE_SDL=2
        -sDISABLE_EXCEPTION_CATCHING=1
    )

    target_link_options(obb_editor PRIVATE
        -sUSE_SDL=2
        -sWASM=1
        -sUSE_WEBGL2=1
        -sWASM=1
        -sALLOW_MEMORY_GROWTH=1
        -sNO_EXIT_RUNTIME=0
        -sASSERTIONS=1
        -sMIN_WEBGL_VERSION=2
        -sMAX_WEBGL_VERSION=2
        -sUSE_WEBGL2=1
        -sFORCE_FILESYSTEM
        -sEXPORTED_RUNTIME_METHODS=ccall
        --shell-file=${CMAKE_CURRENT_LIST_DIR}/resources/shell_minimal.html
    )

    target_link_libraries(obb_editor
    )
else(EMSCRIPTEN)
    # Linux Ubuntu native executable
    target_link_libraries(obb_editor
        dl
        GL
        ${SDL2_LIBRARIES}
    )
endif(EMSCRIPTEN)
