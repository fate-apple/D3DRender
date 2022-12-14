#pragma once

#include "scene/scene.h"
#include "core/camera_controller.h"
#include "core/system.h"
#include "undo_stack.h"
#include "transformation_gizmo.h"
#include "rendering/main_renderer.h"

struct scene_editor
{
	void initialize(editor_scene* scene, main_renderer* renderer);

	bool update(const user_input& input, ldr_render_pass* ldrRenderPass, float dt);

	scene_entity selectedEntity;

	void setEnvironment(const fs::path& filename);

private:
	void drawSettings(float dt);
	void drawMainMenuBar();
	bool drawSceneHierarchy();
	bool handleUserInput(const user_input& input, ldr_render_pass* ldrRenderPass, float dt);
	void drawEntityCreationPopup();

	void updateSelectedEntityUIRotation();

	void setSelectedEntity(scene_entity entity);
	void setSelectedEntityNoUndo(scene_entity entity);

	void serializeToFile();
	bool deserializeFromFile();

	editor_scene* scene;
	main_renderer* renderer;

	undo_stack undoStack;
	transformation_gizmo gizmo;

	camera_controller cameraController;

	vec3 selectedEntityEulerRotation;

	system_info systemInfo;

	friend struct selection_undo;
};
