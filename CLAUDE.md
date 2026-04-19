# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

The project uses CMake with Visual Studio 2022 on Windows. SDK paths are hardcoded in `Engine-Vulkan/CMakeLists.txt`:
- Vulkan SDK: `C:/Programming/Vulkan`
- Qt 6.11.0: `C:/Programming/Qt/6.11.0/msvc2022_64`

```bash
# From repo root
mkdir build && cd build
cmake ../Engine-Vulkan -G "Visual Studio 17 2022"
cmake --build . --config Release
```

Shaders are compiled automatically at build time via `glslc` (from Vulkan SDK). Qt editor deployment runs `windeployqt.exe` post-build.

**Outputs:**
- `Sparkle.exe` — standalone game/engine executable
- `SparkleEditor.exe` — Qt-based editor with embedded Vulkan viewport

## Architecture

### Two executables, one engine library

`EngineLib` (static) contains all engine systems. Both `Sparkle` (game) and `SparkleEditor` (editor) link against it.

Entry points:
- `src/main.cpp` → game loop via `engine_run()`
- `src/editor/main_editor.cpp` → Qt `QApplication` + `MainWindow`

### ECS (Entity Component System)

Defined in `src/engine/ECS.h`. Core concepts:
- **World** — container for all entities and components
- **Archetype** — stores entities with the same component set in column-based arrays (data-oriented)
- **Entity** — `uint32_t` ID
- Query with `world_query<ComponentA, ComponentB>(world, callback)`

Built-in components: `TransformComponent`, `MeshComponent`, `CameraComponent`, light types.

### Rendering pipeline

Deferred rendering with two passes:
1. **Geometry pass** (`geometry.vert/frag`) — writes G-Buffer (position, normal, albedo)
2. **Lighting pass** (`lighting.vert/frag`) — reads G-Buffer, computes lighting

Frame flow: `engine_render()` → `Renderer` builds `FramePacket` from ECS queries → `VulkanContext::draw_frame()` executes both passes.

Vulkan code is split across:
- `VulkanContext` — device, queues, swapchain lifecycle
- `VulkanSwapchain` — surface, framebuffers
- `VulkanPipeline` — graphics pipelines, render passes
- `VulkanResources` — GPU buffers, textures, memory allocation

### Editor (Qt)

`MainWindow` hosts a `VulkanViewport` widget that embeds the Vulkan surface directly into Qt. The editor renders the engine scene inside the Qt window and exposes entity/component inspection.

### Assets

`AssetManager` loads OBJ meshes (via `tiny_obj_loader`) and textures (via `stb_image`). Assets are reference-counted via `AssetHandle`.

## Shaders

Source: `src/shaders/*.vert` / `*.frag` (GLSL 4.50)  
Compiled to SPIR-V `.spv` files at build time — do not edit `.spv` files directly.
