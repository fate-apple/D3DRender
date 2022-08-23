#pragma once
#define ENTT_ID_TYPE uint64
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/entity/helper.hpp>


#include "components.h"
#include "core/Camera.h"
#include "rendering/LightSource.h"
#include "rendering/pbr.h"

// use entt
//https://github.com/skypjack/entt
using entity_handle = entt::entity;
static const auto null_entity = entt::null;

struct scene_entity
{
    scene_entity() = default;
    scene_entity(entity_handle handle, game_scene& scene);
    scene_entity(uint32 id, game_scene& scene);
    scene_entity(entity_handle handle, entt::registry* registry)
        : handle(handle), registry(registry)
    {
    }
    scene_entity(uint32 id, entt::registry* reg)
        : handle((entity_handle)id), registry(reg)
    {
    }
    scene_entity(const scene_entity&) = default;
    scene_entity(scene_entity&&) = delete;

    entity_handle handle = entt::null;
    entt::registry* registry;

    template <typename component_t>
    bool hasComponent()
    {
        return registry->any_of<component_t>(handle);
    }

    template <typename component_t>
    component_t& getComponent()
    {
        return registry->get<component_t>(handle);
    }

    template <typename component_t>
    component_t* getComponentIfExists()
    {
        return registry->try_get<component_t>(handle);
    }
    template <typename TComponent, typename... Targs>
    scene_entity& addComponent(Targs&& ...inArgs)
    {
        //TODO: sjw. callback when added write in component itself;
        if constexpr(std::is_same_v<TComponent, struct collider_component>) {
            void addColliderToBroadphase(scene_entity entity);
            if(!hasComponent<struct physics_reference_component>()) {
                addComponent<struct physics_reference_component>();
            }
            struct physics_reference_component& reference = getComponent<struct physics_reference_component>();
            ++reference.numColliders;
            entity_handle child = registry->create();
            struct collider_component& collider = registry->emplace<struct collider_component>(
                child, std::forward<Targs>(inArgs)...);
            addColliderToBroadphase(scene_entity(child, registry));

            // insert at first
            collider.parentEntity = handle;
            collider.nextEntity = reference.firstColliderEntity;
            reference.firstColliderEntity = child;
            if(struct rigid_body_component* rb = getComponentIfExists<struct rigid_body_component>()) {
                rb->recalculateProperties(registry, reference);
            }
        } else {
            auto& component = registry->emplace_or_replace<TComponent>(handle, std::forward<Targs>(inArgs));
            
            if constexpr (std::is_same_v<TComponent, struct rigid_body_component>)
            {
                if (struct physics_reference_component* ref = getComponentIfExists<struct physics_reference_component>())
                {
                    component.recalculateProperties(registry, *ref);
                }
                if (!hasComponent<dynamic_transform_component>())
                {
                    addComponent<dynamic_transform_component>();
                }
            }
            if constexpr (std::is_same_v<TComponent, struct transform_component>)
            {
                if (struct cloth_component* cloth = getComponentIfExists<struct cloth_component>())
                {
                    cloth->setWorldPositionOfFixedVertices(component, true);
                }
            }
        }
        return *this;
    }
};


struct game_scene
{
    game_scene();

    entt::registry registry;
    fs::path savePath;
    RenderCamera camera;
    DirectionalLight sun;
    ref<PbrEnvironment> environment;

    scene_entity createEntity(const char* name)
    {
        return scene_entity(registry.create(), &registry).addComponent<tag_component>(name);
    }
private:
};

enum SceneState
{
    SceneStateEditor,
    SceneStateRuntimePlaying,
    SceneStateRuntimePaused,
};


struct EditorScene
{
    game_scene editorScene;
    game_scene runtimeScene;
    SceneState state = SceneStateEditor;
    float timeScale = 1.f;

    game_scene& GetCurrentScene()
    {
        return state == SceneStateEditor ? editorScene : runtimeScene;
    }
};