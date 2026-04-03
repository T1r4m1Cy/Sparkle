#include <iostream>

#include "Renderer.h"
#include "CameraSystem.h"
#include "ImGuiLayer.h"
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

    vulkan_init_imgui_render_pass(e.vulkan);
	vulkan_init_imgui_framebuffers(e.vulkan);

	imgui_init(e.vulkan, e.window.handle);
    vulkan_init_offscreen_imgui_descriptors(e.vulkan);

    return 0;
}

void engine_run(Engine& e, bool& stillRunning)
{
    static bool editorMode = true;
    static bool viewportHovered = false;

    //Delta time
    auto now = std::chrono::steady_clock::now();
    e.deltaTime = std::chrono::duration<float>(now - e.lastFrameTime).count();
    e.lastFrameTime = now;

    SDL_Event event;
    float xoffset = 0.0f, yoffset = 0.0f;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL2_ProcessEvent(&event);

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
            event.button.button == SDL_BUTTON_LEFT &&
            viewportHovered) {
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

	imgui_new_frame();

	ImGuiWindowFlags dockspaceFlags = 
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("DockSpace", nullptr, dockspaceFlags);
	ImGui::PopStyleVar(3);

	ImGuiID dockspaceID = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f),
        ImGuiDockNodeFlags_None);

	static bool firstTime = true;
    if (firstTime) {
		firstTime = false;

        ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, 
            ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetMainViewport()->Size);

        ImGuiID dockIDLeft, dockIDCenter, dockIDRight;
        ImGui::DockBuilderSplitNode(
            dockspaceID, ImGuiDir_Left, 0.2f, &dockIDLeft, &dockIDCenter);
        ImGui::DockBuilderSplitNode(
			dockIDCenter, ImGuiDir_Right, 0.25f, &dockIDRight, &dockIDCenter);

		ImGui::DockBuilderDockWindow("World Inspector", dockIDLeft);
		ImGui::DockBuilderDockWindow("Properties", dockIDRight);
		ImGui::DockBuilderDockWindow("Viewport", dockIDCenter);

		ImGui::DockBuilderFinish(dockspaceID);
    }

    ImGui::End();

	ImGui::Begin("Viewport");

	ImVec2 viewportSize = ImGui::GetContentRegionAvail();

	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
	ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
	ImVec2 mousePos = ImGui::GetMousePos();

	viewportHovered = 
        mousePos.x >= windowPos.x + contentMin.x &&
        mousePos.x <= windowPos.x + contentMax.x &&
        mousePos.y >= windowPos.y + contentMin.y &&
		mousePos.y <= windowPos.y + contentMax.y;

    ImGui::Image(
        e.vulkan.offscreenBuffers[e.vulkan.currentFrame].imguiDescriptorSet,
        viewportSize
	);

    ImGui::End();

    static Entity selectedEntity = NULL_ENTITY;

	ImGui::Begin("World Inspector");

	std::string addLabel = "Add Entity";
    if (ImGui::Button(addLabel.c_str())) {
		Entity newEntity = world_create_entity(e.world);
		NameComponent nameComp{ "Entity " + std::to_string(newEntity) };
		world_add_component<NameComponent>(e.world, newEntity, nameComp);
    }
	ImGui::Text("Entities: %d", static_cast<int>(e.world.entityIndex.size()));
	ImGui::Separator();

    for (auto& [entity, record] : e.world.entityIndex) {
		NameComponent* name = world_get_component<NameComponent>(e.world, entity);
		std::string label = name ? name->name : "Entity " + std::to_string(entity);

		bool isSelected = (selectedEntity == entity);
        if (ImGui::Selectable(label.c_str(), isSelected)) {
            selectedEntity = entity;
        }
	}

    ImGui::End();

    ImGui::Begin("Properties");
    
    if (selectedEntity != NULL_ENTITY) {
        auto& reg = get_component_registry();

		auto& rec = e.world.entityIndex[selectedEntity];
        Archetype* arch = e.world.archetypes[rec.archetypeID].get();

        for (ComponentID id : arch->componentIDs) {
            auto it = reg.components.find(id);
            if (it == reg.components.end()) {
                continue;
			}

            ComponentInfo& info = it->second;

			int colIdx = archetype_column_index(*arch, id);
			void* compPtr = column_get(arch->columns[colIdx], rec.row);

            if (ImGui::CollapsingHeader(info.name.c_str(),
                ImGuiTreeNodeFlags_DefaultOpen)) {
                for (auto& field : info.fields) {
                    field.drawImGui(compPtr);
				}
            }

			std::string removeLabel = "Remove##" + info.name;
            if (ImGui::Button(removeLabel.c_str())) {
                info.remove(e.world, selectedEntity);
                break;
            }
		}

        ImGui::Separator();

        if (ImGui::Button("Add Component", ImVec2(-1, 0))) {
            ImGui::OpenPopup("AddComponentPopup");
        }

        if (ImGui::BeginPopup("AddComponentPopup")) {
            for (auto& [id, info] : reg.components) {
                if (archetype_column_index(*arch, id) == -1) {
                    if (ImGui::MenuItem(info.name.c_str())) {
                        info.add(e.world, selectedEntity);
						auto& rec2 = e.world.entityIndex[selectedEntity];
						arch = e.world.archetypes[rec2.archetypeID].get();
						ImGui::CloseCurrentPopup();
                        break;
                    }
				}
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();

	float aspectRatio = viewportSize.x / viewportSize.y;

    draw_frame(e.vulkan, e.assets, e.window.handle, 
        update_renderer(e.world, e.assets, aspectRatio));

    //SDL_Delay(10);
}

void engine_shutdown(Engine& e)
{
    e.vulkan.device.waitIdle();

	imgui_shutdown(e.vulkan);
    e.assets.assets_shutdown(e.vulkan);
	vulkan_shutdown(e.vulkan);
	window_shutdown(e.window);
}