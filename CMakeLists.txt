cmake_minimum_required(VERSION 3.16)
project(OverlayLabel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES
    src/main.cpp
    src/config.cpp
    src/autostart.cpp
    src/overlay.cpp
    src/settings_window.cpp
    src/label_printer.cpp
    src/print_dialog.cpp
)

if(WIN32)
    list(APPEND SOURCES resources/app.rc)
endif()

add_executable(OverlayLabel WIN32 ${SOURCES})

target_include_directories(OverlayLabel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
)

target_compile_definitions(OverlayLabel PRIVATE
    UNICODE
    _UNICODE
)

if(MSVC)
    # Use static CRT so the output .exe requires zero external redistributable DLLs
    set_property(TARGET OverlayLabel PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(OverlayLabel PRIVATE /W4 /utf-8)
    # Manifest is already embedded via resources/app.rc
    target_link_options(OverlayLabel PRIVATE /MANIFEST:NO)
elseif(MINGW)
    target_compile_options(OverlayLabel PRIVATE -Wall -Wextra -municode)
    target_link_options(OverlayLabel PRIVATE -municode -mwindows -static -static-libgcc -static-libstdc++)
endif()

target_link_libraries(OverlayLabel PRIVATE
    user32
    gdi32
    gdiplus
    shell32
    advapi32
    comctl32
    comdlg32
    ole32
)
