// SDL_MAIN_HANDLED prevents SDL from renaming main() to SDL_main,
// which would conflict with Qt's own entry point plumbing.
#define SDL_MAIN_HANDLED

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <QCoreApplication>
#include <QTimer>
#include <iostream>

#include "Engine.h"
#include "VulkanContext.h"
#include "VulkanSwapchain.h"
#include "RuntimeServer.h"
#include "RuntimeWindowProvider.h"
#include "ComponentBuilder.h"
#include "ComponentRegistry.h"

// Replace the current Vulkan surface with one created from a new HWND.
// Must be called between frames (engine is idle).
static void switch_surface(Engine& engine, RuntimeWindowProvider& provider,
                           HWND newHwnd, uint32_t w, uint32_t h)
{
    engine.vulkan.device.waitIdle();

    vulkan_cleanup_swapchain(engine.vulkan);
    engine.vulkan.instance.destroySurfaceKHR(engine.vulkan.surface);

    provider.use_external_hwnd(newHwnd, w, h);
    engine.vulkan.surface = provider.create_surface(engine.vulkan.instance);

    vulkan_recreate_swapchain(engine.vulkan, { w, h });

    std::cout << "[Runtime] Surface switched → " << w << "x" << h << "\n";
}

static void recreate_surface(Engine& engine, RuntimeWindowProvider& provider,
                              uint32_t w, uint32_t h)
{
    engine.vulkan.device.waitIdle();
    provider.width  = w;
    provider.height = h;
    vulkan_recreate_swapchain(engine.vulkan, { w, h });
    std::cout << "[Runtime] Swapchain resized → " << w << "x" << h << "\n";
}

static void runtime_shutdown(Engine& e)
{
    e.vulkan.device.waitIdle();
    e.assets.assets_shutdown(e.vulkan);
    vulkan_shutdown(e.vulkan);
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    constexpr uint32_t WIDTH  = 1280;
    constexpr uint32_t HEIGHT = 720;
    constexpr uint16_t PORT   = 57300;

    Engine engine;
    RuntimeWindowProvider provider(WIDTH, HEIGHT);
    provider.vulkan = &engine.vulkan;
    engine.windowProvider = &provider;

    register_engine_components();

    if (engine_init(engine, WIDTH, HEIGHT) != 0) {
        std::cerr << "[Runtime] Engine init failed\n";
        return 1;
    }
    std::cout << "[Runtime] Engine ready\n";

    RuntimeServer server(PORT);

    QObject::connect(&server, &RuntimeServer::shutdownRequested,
                     &app, &QCoreApplication::quit);

    QObject::connect(&server, &RuntimeServer::viewportChanged,
                     [&](quint64 hwnd, quint32 w, quint32 h) {
        switch_surface(engine, provider, reinterpret_cast<HWND>(hwnd), w, h);
    });

    QObject::connect(&server, &RuntimeServer::resizeRequested,
                     [&](quint32 w, quint32 h) {
        recreate_surface(engine, provider, w, h);
    });

    // Run engine as fast as Qt's event loop allows.
    QTimer frameTimer;
    QObject::connect(&frameTimer, &QTimer::timeout, [&]() {
        engine_tick(engine);
        engine_render(engine);
    });
    frameTimer.start(0);

    auto send_world_snapshot = [&]() {
        QByteArray payload;
        uint32_t count = 0;

        for (auto& [entity, record] : engine.world.entityIndex) {
            auto* meta = world_get_component<EntityMetaComponent>(engine.world, entity);
            EntityInfo info{};
            info.id       = entity;
            info.parentId = meta ? meta->parentId : UINT32_MAX;
            strncpy(info.name, meta ? meta->name : "Entity", 63);
            payload.append(reinterpret_cast<const char*>(&info), sizeof(info));
            count++;
        }

        QByteArray full;
        full.append(reinterpret_cast<const char*>(&count), sizeof(count));
        full.append(payload);
        server.send(MsgType::WorldSnapshot, full);
    };

    QObject::connect(&server, &RuntimeServer::entitySelected, [&](quint32 id) {
        ComponentBuilder b(id);
        for (auto& [cid, info] : get_component_registry().components) {
            void* ptr = info.get(engine.world, id);
            if (!ptr) continue;
            b.begin_component(info.name.c_str());
            for (auto& field : info.fields)
                if (field.serialize) field.serialize(ptr, b);
        }
        server.send(MsgType::EntityComponents, b.build());
    });

    QObject::connect(&server, &RuntimeServer::fieldChanged,
                     [&](quint32 id, QString compName, QString fieldName,
                         quint32 /*fieldType*/, QByteArray valueBytes) {
        const float* f = reinterpret_cast<const float*>(valueBytes.constData());
        get_component_registry().apply_field(
            engine.world, id,
            compName.toStdString(), fieldName.toStdString(), f);
        if (compName == "EntityMetaComponent")
            engine.world.notify_world_changed();
    });

    QObject::connect(&server, &RuntimeServer::parentChangeRequested, [&](quint32 id, quint32 parentId) {
        auto* meta = world_get_component<EntityMetaComponent>(engine.world, id);
        if (meta) {
            meta->parentId = parentId;
            engine.world.notify_world_changed();
        }
    });

    QObject::connect(&server, &RuntimeServer::entityRenameRequested, [&](quint32 id, QString name) {
        auto* meta = world_get_component<EntityMetaComponent>(engine.world, id);
        if (meta) {
            auto utf8 = name.toUtf8();
            strncpy(meta->name, utf8.constData(), sizeof(meta->name) - 1);
            meta->name[sizeof(meta->name) - 1] = '\0';
            engine.world.notify_world_changed();
        }
    });

    QObject::connect(&server, &RuntimeServer::entityCreateRequested, [&](QString name) {
        Entity e = world_create_entity(engine.world);
        EntityMetaComponent meta{};
        auto utf8 = name.toUtf8();
        strncpy(meta.name, utf8.constData(), sizeof(meta.name) - 1);
        meta.name[sizeof(meta.name) - 1] = '\0';
        world_add_component(engine.world, e, meta);
    });

    // Send current state when editor connects (catches entities created during engine_init).
    QObject::connect(&server, &RuntimeServer::editorConnected, [&]() {
        send_world_snapshot();
    });

    // Send updated state whenever the world changes.
    engine.world.onWorldChanged.push_back([&](World&) {
        send_world_snapshot();
    });

    int result = app.exec();

    runtime_shutdown(engine);
    return result;
}
