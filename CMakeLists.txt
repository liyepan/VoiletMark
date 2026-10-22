cmake_minimum_required(VERSION 3.16)

project(VoiletMark VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets)

set(VM_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MarkdownEditor.cpp
    src/MarkdownHighlighter.cpp
    src/ThemeManager.cpp
)

set(VM_HEADERS
    src/MainWindow.h
    src/MarkdownEditor.h
    src/MarkdownHighlighter.h
    src/ThemeManager.h
)

add_executable(VoiletMark WIN32 MACOSX_BUNDLE ${VM_SOURCES} ${VM_HEADERS})

target_link_libraries(VoiletMark PRIVATE Qt5::Widgets)
target_compile_definitions(VoiletMark PRIVATE VM_VERSION="${PROJECT_VERSION}")

install(TARGETS VoiletMark
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .)
