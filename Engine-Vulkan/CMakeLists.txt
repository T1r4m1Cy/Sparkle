cmake_minimum_required(VERSION 3.20)
project(Sparkle)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VULKAN_SDK "C:/Programming/Vulkan")
set(CMAKE_PREFIX_PATH "C:/Programming/Qt/6.11.0/msvc2022_64")

find_package(Vulkan REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# ─── Исходники движка ──────────────────────────────────────────────
file(GLOB ENGINE_SOURCES
    src/engine/*.cpp
    src/engine/*.h
)

file(GLOB EDITOR_SOURCES
    src/editor/*.cpp
    src/editor/*.h
)

# ─── Engine Library ────────────────────────────────────────────────
add_library(EngineLib STATIC
    ${ENGINE_SOURCES}
)

target_include_directories(EngineLib PUBLIC
    src/engine
    ${Vulkan_INCLUDE_DIRS}
    ${VULKAN_SDK}/Include
    ${VULKAN_SDK}/Include/SDL2
)

target_link_directories(EngineLib PUBLIC ${VULKAN_SDK}/Lib)

target_link_libraries(EngineLib PUBLIC
    Vulkan::Vulkan
    SDL2
    SDL2main
)

# ─── Game ──────────────────────────────────────────────────────────
add_executable(Sparkle
    src/main.cpp
)

target_link_libraries(Sparkle PRIVATE EngineLib)

add_custom_command(TARGET Sparkle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${VULKAN_SDK}/Bin/SDL2.dll"
        $<TARGET_FILE_DIR:Sparkle>
)

# ─── Editor ────────────────────────────────────────────────────────
add_executable(SparkleEditor
    ${EDITOR_SOURCES}
)

target_include_directories(SparkleEditor PRIVATE
    src/editor
)

target_link_libraries(SparkleEditor PRIVATE
    EngineLib
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Vulkan::Vulkan
)

# ─── Шейдеры ───────────────────────────────────────────────────────
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
add_custom_command(TARGET SparkleEditor POST_BUILD
    COMMAND "C:/Programming/Qt/6.11.0/msvc2022_64/bin/windeployqt.exe"
        $<TARGET_FILE:SparkleEditor>
)
add_dependencies(Sparkle Shaders)