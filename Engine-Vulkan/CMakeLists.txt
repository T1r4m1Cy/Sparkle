cmake_minimum_required(VERSION 3.20)
project(Sparkle)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Пути к зависимостям ───────────────────────────────────────────
set(VULKAN_SDK "C:/Programming/Vulkan")
set(CMAKE_PREFIX_PATH "C:/Programming/Qt/6.11.0/msvc2022_64")

# ─── Vulkan ────────────────────────────────────────────────────────
find_package(Vulkan REQUIRED)
# Если find_package не найдёт — раскомментируй и убери строку выше:
# set(Vulkan_LIBRARIES "${VULKAN_SDK}/Lib/vulkan-1.lib")
# set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK}/Include")

# ─── Qt ────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# ─── Исходники движка ──────────────────────────────────────────────
file(GLOB ENGINE_SOURCES
    src/engine/*.cpp
    src/engine/*.h
)

# ─── Исполняемый файл ──────────────────────────────────────────────
add_executable(Sparkle
    src/main.cpp
    ${ENGINE_SOURCES}
)

set_target_properties(Sparkle PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_include_directories(Sparkle PRIVATE
    src/engine
    ${Vulkan_INCLUDE_DIRS}
    ${VULKAN_SDK}/Include       # SDL2/, glm/ находятся здесь
    ${VULKAN_SDK}/Include/SDL2   # для #include <SDL2/SDL.h>
)

target_link_directories(Sparkle PRIVATE
    ${VULKAN_SDK}/Lib
)

target_link_libraries(Sparkle PRIVATE
    Vulkan::Vulkan
    SDL2
    SDL2main
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)

# ─── Копировать SDL2.dll рядом с exe после сборки ──────────────────
add_custom_command(TARGET Sparkle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${VULKAN_SDK}/Bin/SDL2.dll"
        $<TARGET_FILE_DIR:Sparkle>
)

# ─── Компиляция шейдеров ───────────────────────────────────────────
find_program(GLSLC glslc HINTS "${VULKAN_SDK}/Bin")

file(GLOB_RECURSE GLSL_SOURCES
    shaders/*.vert
    shaders/*.frag
)

foreach(GLSL ${GLSL_SOURCES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSLC} ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_FILES ${SPIRV})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_FILES})
add_dependencies(Sparkle Shaders)