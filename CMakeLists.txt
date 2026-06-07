cmake_minimum_required(VERSION 3.15)
project(overlaykit VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/overlay_window.cpp
    src/hotkey_manager.cpp
    src/drawing_engine.cpp
    src/toolbar.cpp
    src/tray.cpp
    src/config_manager.cpp
    src/settings_window.cpp
    src/pinned_panel.cpp
)

if(WIN32)
    list(APPEND SOURCES src/platform_windows.cpp)
else()
    list(APPEND SOURCES src/platform_linux.cpp)
endif()

# Header files (for IDE visibility and completeness)
set(HEADERS
    include/main.h
    include/overlay_window.h
    include/hotkey_manager.h
    include/drawing_engine.h
    include/toolbar.h
    include/tray.h
    include/config_manager.h
    include/settings_window.h
    include/pinned_panel.h
    include/platform.h
)

# Include directories
include_directories(include)

# Subsystem WIN32 prevents a console window from opening on app launch
add_executable(overlaykit WIN32 ${SOURCES} ${HEADERS})

# Target-specific compiler options (warnings-as-errors style can be set if needed, but standard warnings are enabled here)
if(MSVC)
    target_compile_options(overlaykit PRIVATE /W4 /WX)
else()
    target_compile_options(overlaykit PRIVATE -Wall -Wextra -Werror)
endif()

# Link necessary libraries
target_link_libraries(overlaykit PRIVATE
    gdi32
    user32
    gdiplus
    shlwapi
    dwmapi
)
