#include <iostream>

#include "Renderer.h"
#include "CameraSystem.h"
#include "ComponentRegistry.h"

#include "Engine.h"

static constexpr float MOUSE_SENSIVITY = 0.1f;
static constexpr float CAMERA_SPEED = 3.0f;

int engine_init(Engine& e, int width, int height)
{
    if (window_init(e.window, width, height) == 1) return 1;
    if (vulkan_init(e.vulkan, e.window.handle) == 1) return 1;

    e.world = world_create();

    init_renderer(e.world.onWorldChanged);

	auto mesh1 = e.assets.load_mesh(e.vulkan, "models/viking_room.obj");
    auto texture1 = e.assets.load_texture(e.vulkan, "textures/viking_room.png");

    auto mesh2 = e.assets.load_mesh(e.vulkan, "models/Bulbasaur1.obj");
    auto texture2 = e.assets.load_texture(e.vulkan, "textures/Bulba_D.tga.png");

    Entity e0 = world_create_entity(e.world);

    NameComponent n0 = { "Camera" };

    TransformComponent t0 = {
        {3.0f, 3.0f, 2.0f},
        glm::angleAxis(glm::radians(0.0f), glm::vec3(1, 0, 0)),
        {1,1,1}
    };

    CameraComponent c0 = {
        -135.0f,
        -30.0f,
        60.0f,
        0.1f,
        100.0f,
        true
    };

    world_add_component(e.world, e0, n0);
    world_add_component(e.world, e0, t0);
    world_add_component(e.world, e0, c0);

    camera_set_active(e.world, e0);

    Entity e1 = world_create_entity(e.world);
    Entity e2 = world_create_entity(e.world);

    Entity e3 = world_create_entity(e.world);
    Entity e4 = world_create_entity(e.world);

    NameComponent n3 = { "Point Light 1" };

    TransformComponent t3 = {
        {0.0f, 4.0f, 0.0f},
        glm::angleAxis(glm::radians(0.0f), glm::vec3(1, 0, 0)),
        {1,1,1}
    };
    PointLightComponent pl3 = { {1.0f, 1.0f, 1.0f}, 0.8f };

    NameComponent n4 = { "Point Light 2" };

    TransformComponent t4 = {
        {4.0f, 0.0f, 0.0f},
        glm::angleAxis(glm::radians(0.0f), glm::vec3(1, 0, 0)),
        {1,1,1}
    };
    PointLightComponent pl4 = { {1.0f, 1.0f, 1.0f}, 0.8f };

	world_add_component(e.world, e3, n3);
    world_add_component(e.world, e3, t3);
    world_add_component(e.world, e3, pl3);

    world_add_component(e.world, e4, t4);
    world_add_component(e.world, e4, n4);
    world_add_component(e.world, e4, pl4);

    NameComponent n1 = { "Viking Room" };

    TransformComponent t1 = { 
        {0.0f, 0.0f, 0.0f}, 
        glm::angleAxis(glm::radians(0.0f), glm::vec3(1, 0, 0)),
        {1,1,1} 
    };
    MeshComponent m1 = { mesh1, texture1 };

    NameComponent n2 = { "Bulbasaur" };

    TransformComponent t2 = {
        {0.0f, 0.0f, 0.0f},
        glm::angleAxis(glm::radians(90.0f), glm::vec3(1, 0, 0)),
        {1,1,1}
    };
    MeshComponent m2 = { mesh2, texture2 };

    world_add_component(e.world, e1, n1);
    world_add_component(e.world, e1, m1);
    world_add_component(e.world, e1, t1);

    world_add_component(e.world, e2, t2);
    world_add_component(e.world, e2, m2);
    world_add_component(e.world, e2, n2);

    //world_remove_component<PointLightComponent>(e.world, e1);
    //world_remove_component<PointLightComponent>(e.world, e4);
    //world_remove_component<TransformComponent>(e.world, e1);
    //world_remove_component<MeshComponent>(e.world, e3);

    debug_world(e.world);

    return 0;
}

void engine_run(Engine& e, bool& stillRunning)
{
    static bool editorMode = true;

    //Delta time
    auto now = std::chrono::steady_clock::now();
    e.deltaTime = std::chrono::duration<float>(now - e.lastFrameTime).count();
    e.lastFrameTime = now;

    SDL_Event event;
    float xoffset = 0.0f, yoffset = 0.0f;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT) {
            stillRunning = false;
        }

        if (event.type == SDL_KEYDOWN &&
            event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            if (!editorMode) {
                editorMode = true;
            }
        }

        if (event.type == SDL_MOUSEBUTTONDOWN && 
            event.button.button == SDL_BUTTON_LEFT) {
            editorMode = false;
		}

        if (!editorMode && event.type == SDL_MOUSEMOTION) {
            xoffset = static_cast<float>(event.motion.xrel) * MOUSE_SENSIVITY;
            yoffset = static_cast<float>(event.motion.yrel) * MOUSE_SENSIVITY;
            camera_rotate(e.world, xoffset, yoffset);
        }
    }

    if (editorMode) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_ShowCursor(SDL_ENABLE);
    }
    else {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_ShowCursor(SDL_DISABLE);
    }

    if (!editorMode) {
        const Uint8* state = SDL_GetKeyboardState(NULL);
        glm::vec3 forward = camera_get_forward(e.world);
        glm::vec3 right = camera_get_right(e.world);

        if (state[SDL_SCANCODE_W]) {
            camera_move(e.world, forward, CAMERA_SPEED * e.deltaTime);
        }
        if (state[SDL_SCANCODE_S]) {
            camera_move(e.world, -forward, CAMERA_SPEED * e.deltaTime);
        }
        if (state[SDL_SCANCODE_A]) {
            camera_move(e.world, -right, CAMERA_SPEED * e.deltaTime);
        }
        if (state[SDL_SCANCODE_D]) {
            camera_move(e.world, right, CAMERA_SPEED * e.deltaTime);
        }
    }

	float aspectRatio = e.vulkan.swapchain.swapchainExtent.width / 
        (float)e.vulkan.swapchain.swapchainExtent.height;

    draw_frame(e.vulkan, e.assets, e.window.handle, 
        update_renderer(e.world, e.assets, aspectRatio));

    //SDL_Delay(10);
}

void engine_shutdown(Engine& e)
{
    e.vulkan.device.waitIdle();

    e.assets.assets_shutdown(e.vulkan);
	vulkan_shutdown(e.vulkan);
	window_shutdown(e.window);
}