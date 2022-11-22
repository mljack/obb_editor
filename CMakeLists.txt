cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

project(obb_editor CXX)
set(CMAKE_CXX_STANDARD 17)
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
	set(CMAKE_CXX_COMPILER /usr/bin/g++-9)
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
	src/texture.cpp
	src/ui.cpp
)

include_directories(
	${SDL2_INCLUDE_DIRS}
	third_party
	third_party/glm
	third_party/imgui
	third_party/imgui/backends
)

if(EMSCRIPTEN)
	# WebAssembly
	set(CMAKE_EXECUTABLE_SUFFIX .html)

	target_compile_options(obb_editor PRIVATE
		-sUSE_SDL=2
		#-sDISABLE_EXCEPTION_CATCHING=1
	)

	target_link_options(obb_editor PRIVATE
		-sWASM=1
		-sALLOW_MEMORY_GROWTH=1
		-sNO_DISABLE_EXCEPTION_CATCHING=1
		-sNO_EXIT_RUNTIME=0
		-sASSERTIONS=1
		-sUSE_SDL=2
		-sUSE_WEBGL2=1
		-sMIN_WEBGL_VERSION=2
		-sMAX_WEBGL_VERSION=2
		-sFORCE_FILESYSTEM
		-sASYNCIFY
		-sEXPORTED_RUNTIME_METHODS=[ccall,cwrap]
		-sEXPORTED_FUNCTIONS=[_main,_new_filelist,_add_folder,_add_file,_pop_folder]
		--preload-file=${CMAKE_CURRENT_LIST_DIR}/resources/cat.jpg@/resources/cat.jpg
		--preload-file=${CMAKE_CURRENT_LIST_DIR}/resources/bird.jpg@/images/bird.jpg
		--preload-file=${CMAKE_CURRENT_LIST_DIR}/resources/dog.jpg@/images/dog.jpg
		--shell-file=${CMAKE_CURRENT_LIST_DIR}/resources/shell_minimal.html
	)

	add_custom_target(copy-runtime-files ALL COMMAND ${CMAKE_COMMAND}
		-E copy_directory
		${CMAKE_SOURCE_DIR}/resources/inline-scripts
		${CMAKE_BINARY_DIR}/inline-scripts
		DEPENDS obb_editor)

else(EMSCRIPTEN)
	# Linux Ubuntu native executable
	target_link_libraries(obb_editor
		dl
		GL
		${SDL2_LIBRARIES}
	)
endif(EMSCRIPTEN)
