cmake_minimum_required(VERSION 3.21)

if(NOT WIN32)
    message(FATAL_ERROR "uShader targets Windows 10/11 only.")
endif()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" USHADER_VERSION_RAW LIMIT_COUNT 1)
string(STRIP "${USHADER_VERSION_RAW}" USHADER_VERSION_RAW)

project(ushader VERSION ${USHADER_VERSION_RAW} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Git QUIET)
set(APP_BUILD 0)
if(Git_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE APP_BUILD_RAW
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE APP_BUILD_RESULT
    )
    if(APP_BUILD_RESULT EQUAL 0 AND APP_BUILD_RAW)
        set(APP_BUILD ${APP_BUILD_RAW})
    endif()
endif()

set(APP_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(APP_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(APP_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(APP_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${APP_BUILD}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/ushader/version.h
    @ONLY
)

include(FetchContent)

set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.18
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.8-docking
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
    imguicolortextedit
    GIT_REPOSITORY https://github.com/BalazsJako/ImGuiColorTextEdit.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imguicolortextedit)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC SDL3::SDL3)

add_library(imguicolortextedit STATIC
    ${imguicolortextedit_SOURCE_DIR}/TextEditor.cpp
)
target_include_directories(imguicolortextedit PUBLIC
    ${imguicolortextedit_SOURCE_DIR}
)
target_link_libraries(imguicolortextedit PUBLIC imgui)

set(RUST_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rust-core)
set(RUST_CORE_LIB ${RUST_CORE_DIR}/target/release/ushader_core.lib)
file(GLOB_RECURSE RUST_CORE_SOURCES ${RUST_CORE_DIR}/src/*.rs)

find_program(CARGO_EXECUTABLE cargo REQUIRED)

add_custom_command(
    OUTPUT ${RUST_CORE_LIB}
    COMMAND ${CARGO_EXECUTABLE} build --release --features capi
    WORKING_DIRECTORY ${RUST_CORE_DIR}
    DEPENDS ${RUST_CORE_DIR}/Cargo.toml ${RUST_CORE_SOURCES}
    COMMENT "Building rust-core"
    VERBATIM
)
add_custom_target(rust_core_build DEPENDS ${RUST_CORE_LIB})

add_library(rust_core STATIC IMPORTED GLOBAL)
set_target_properties(rust_core PROPERTIES IMPORTED_LOCATION ${RUST_CORE_LIB})
add_dependencies(rust_core rust_core_build)
target_include_directories(rust_core INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(rust_core INTERFACE ws2_32 userenv advapi32 bcrypt ntdll)

add_executable(ushader
    src/main.cpp
    src/render/gl_functions.cpp
    src/render/shader_runner.cpp
    src/render/framebuffer.cpp
    src/platform/paths.cpp
    src/ui/theme.cpp
    src/ui/layout.cpp
    src/ui/glsl_language.cpp
    src/ui/glsl_format.cpp
)

target_include_directories(ushader PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${stb_SOURCE_DIR}
)

target_link_libraries(ushader PRIVATE
    SDL3::SDL3
    imgui
    imguicolortextedit
    opengl32
    rust_core
)

add_custom_command(
    TARGET ushader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:ushader>/assets
)

add_executable(rust_core_smoke_test
    tests/rust_core_smoke_test.cpp
)

target_link_libraries(rust_core_smoke_test PRIVATE rust_core)
