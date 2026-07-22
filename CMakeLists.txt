cmake_minimum_required(VERSION 3.21)

if(NOT WIN32)
    message(FATAL_ERROR "uShader targets Windows 10/11 only.")
endif()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" USHADER_VERSION_RAW LIMIT_COUNT 1)
string(STRIP "${USHADER_VERSION_RAW}" USHADER_VERSION_RAW)

project(ushader VERSION ${USHADER_VERSION_RAW} LANGUAGES CXX RC)

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

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

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

add_executable(rust_core_smoke_test
    tests/rust_core_smoke_test.cpp
)

target_link_libraries(rust_core_smoke_test PRIVATE rust_core)

add_executable(golf_profile_roundtrip_test
    tests/golf_profile_roundtrip_test.cpp
    src/ui/golf_profile.cpp
    src/ui/budget_presets.cpp
    src/platform/paths.cpp
    src/platform/utf8.cpp
)

target_link_libraries(golf_profile_roundtrip_test PRIVATE
    rust_core
)

target_compile_definitions(golf_profile_roundtrip_test PRIVATE
    USHADER_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/sample.ushaderprofile"
)

add_executable(workspace_roundtrip_test
    tests/workspace_roundtrip_test.cpp
    src/ui/workspace.cpp
    src/ui/golf_profile.cpp
    src/ui/budget_presets.cpp
    src/platform/paths.cpp
    src/platform/utf8.cpp
)

target_link_libraries(workspace_roundtrip_test PRIVATE
    rust_core
)

add_executable(report_encoding_test
    tests/report_encoding_test.cpp
    src/report/report_encoding.cpp
)

add_executable(export_wrappers_test
    tests/export_wrappers_test.cpp
    src/ui/export_wrappers.cpp
)

add_executable(exclude_list_import_test
    tests/exclude_list_import_test.cpp
    src/ui/exclude_list_import.cpp
    src/platform/paths.cpp
    src/platform/file_dialog.cpp
    src/platform/utf8.cpp
)

target_link_libraries(exclude_list_import_test PRIVATE
    comdlg32
)

add_executable(ushaderprofile_schema_test
    tests/ushaderprofile_schema_test.cpp
    src/ui/golf_profile.cpp
    src/ui/budget_presets.cpp
    src/platform/paths.cpp
    src/platform/utf8.cpp
)

target_link_libraries(ushaderprofile_schema_test PRIVATE
    rust_core
)

add_executable(wgl_viewport_spike
    src/render/wgl_viewport_spike_main.cpp
    src/render/wgl_viewport_host.cpp
    src/render/gl_functions_wgl.cpp
    src/render/gl_functions_globals.cpp
    src/render/shader_runner.cpp
    src/render/framebuffer.cpp
    src/platform/screenshot.cpp
    src/platform/stb_impl.cpp
)

target_include_directories(wgl_viewport_spike PRIVATE
    ${stb_SOURCE_DIR}
)

target_link_libraries(wgl_viewport_spike PRIVATE
    opengl32
    gdi32
    user32
)

add_executable(ushader
    src/main_win32.cpp
    src/render/wgl_viewport_host.cpp
    src/render/gl_functions_wgl.cpp
    src/render/gl_functions_globals.cpp
    src/render/shader_runner.cpp
    src/render/shader_error_state.cpp
    src/render/framebuffer.cpp
    src/platform/screenshot.cpp
    src/platform/stb_impl.cpp
    src/platform/utf8.cpp
    src/platform/accessibility_core.cpp
    src/ui/win32_theme_brushes.cpp
    src/ui/win32_title_bar.cpp
    src/ui/win32_tab_strip.cpp
    src/ui/win32_icon_set.cpp
    src/ui/win32_status_dot.cpp
    src/ui/win32_animation.cpp
    src/ui/win32_text_editor.cpp
    src/ui/win32_minimap.cpp
    src/ui/win32_diff_view.cpp
    src/ui/win32_trace_view.cpp
    src/ui/win32_command_palette.cpp
    src/ui/win32_golf_controls.cpp
    src/ui/win32_stats_panel.cpp
    src/ui/win32_appearance_settings.cpp
    src/ui/win32_appearance_panel.cpp
    src/ui/win32_about_panel.cpp
    src/ui/win32_tool_button.cpp
    src/ui/golf_options_convert.cpp
    src/ui/budget_presets.cpp
    src/ui/unified_diff.cpp
    src/ui/golf_trace.cpp
    src/ui/glsl_token_rules.cpp
    src/ui/glsl_syntax_colors.cpp
    src/ui/glsl_format.cpp
    src/ui/fuzzy_match.cpp
    src/ui/keybindings_storage.cpp
    src/ui/win32_keybindings.cpp
    src/ui/recent_files.cpp
    src/platform/paths.cpp
    src/platform/file_dialog.cpp
    assets/icons/app.rc
)

target_include_directories(ushader PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${stb_SOURCE_DIR}
    src
)

target_link_libraries(ushader PRIVATE
    opengl32
    gdi32
    user32
    shell32
    comdlg32
    d2d1
    dwrite
    dwmapi
    gdiplus
    uiautomationcore
    ole32
    oleaut32
    rust_core
)

if(MSVC)
    target_link_options(ushader PRIVATE "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup")
endif()

add_custom_command(
    TARGET ushader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:ushader>/assets
)

add_executable(wgl_equivalence_test
    tests/wgl_equivalence_test.cpp
    src/render/wgl_viewport_host.cpp
    src/render/gl_functions_wgl.cpp
    src/render/gl_functions_globals.cpp
    src/render/shader_runner.cpp
    src/render/framebuffer.cpp
    src/platform/screenshot.cpp
    src/platform/stb_impl.cpp
)

target_include_directories(wgl_equivalence_test PRIVATE
    ${stb_SOURCE_DIR}
)

target_link_libraries(wgl_equivalence_test PRIVATE
    opengl32
    gdi32
    user32
)
