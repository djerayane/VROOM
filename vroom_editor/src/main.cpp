#include <vroom/core/Engine.hpp>
#include <vroom/components/Camera.hpp>
#include <vroom/components/MeshRenderer.hpp>
#include <vroom/components/MeshFilter.hpp>
#include <vroom/components/Transform.hpp>
#include <vroom/asset/Mesh.hpp>
#include <vroom/asset/Material.hpp>
#include <vroom/asset/AssetManager.hpp>
#include <vroom/asset/AssetProvider.hpp>
#include <glm/glm.hpp>
#include <vroom/logging/LogMacros.hpp>
#ifdef VROOM_WITH_IMGUI
#include <imgui.h>
#endif

int main()
{
    vroom::Engine engine;
    engine.getAssetManager().addProvider(std::make_unique<vroom::DiskAssetProvider>("vroom_editor/assets"));
    auto editorScene = std::make_shared<vroom::Scene>();
    editorScene->setSceneManager(&engine.getSceneManager());
    // Camera entity positioned behind and slightly above the origin, looking forward.
    auto cameraEntity = editorScene->createEntity(glm::vec3(0.0f, 1.0f, 5.0f));
    auto& camera = cameraEntity->addComponent<vroom::Camera>();
    camera.setFieldOfView(60.0f);
    camera.setNearClipPlane(0.1f);
    camera.setFarClipPlane(1000.0f);
    // Mesh entity at the origin.
    auto editorEntity = editorScene->createEntity(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f));
    auto& meshFilter = editorEntity->addComponent<vroom::MeshFilter>();
    auto mesh = engine.getAssetManager().getAsset<vroom::Mesh>("suzanne.obj");
    if (mesh)
    {
        size_t vertexCount = mesh->getVertices().size();
        size_t indexCount = mesh->getIndices().size();
        LOG_INFO("Main", "Loaded suzanne.obj with " + std::to_string(vertexCount) + " vertices and " + std::to_string(indexCount) + " indices");
        meshFilter.setMesh(mesh);
    }
    else
    {
        LOG_ERROR("Main", "Failed to load suzanne.obj");
    }
    auto& meshRenderer = editorEntity->addComponent<vroom::MeshRenderer>();
    auto defaultMaterial = std::make_shared<vroom::Material>();
    defaultMaterial->setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    meshRenderer.setMaterial(defaultMaterial);
    engine.getSceneManager().loadScene(editorScene);
#ifdef VROOM_WITH_IMGUI
    engine.setImGuiCallback([&editorEntity, &cameraEntity]() {
        ImGui::Begin("Transform");
        auto* transform = editorEntity->getComponent<vroom::Transform>();
        if (transform) {
            glm::vec3 pos = transform->getPosition();
            glm::vec3 rot = transform->getRotation();
            glm::vec3 scale = transform->getScale();
            if (ImGui::DragFloat3("Position", &pos.x, 0.01f))
                transform->setPosition(pos);
            if (ImGui::DragFloat3("Rotation", &rot.x, 1.0f))
                transform->setRotation(rot);
            if (ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.001f))
                transform->setScale(scale);
        }
        ImGui::End();
        ImGui::Begin("Camera");
        auto* camTransform = cameraEntity->getComponent<vroom::Transform>();
        auto* cam = cameraEntity->getComponent<vroom::Camera>();
        if (camTransform) {
            glm::vec3 pos = camTransform->getPosition();
            glm::vec3 rot = camTransform->getRotation();
            if (ImGui::DragFloat3("Position##cam", &pos.x, 0.05f))
                camTransform->setPosition(pos);
            if (ImGui::DragFloat3("Rotation##cam", &rot.x, 1.0f))
                camTransform->setRotation(rot);
        }
        if (cam) {
            float fov = cam->getFieldOfView();
            float nearPlane = cam->getNearClipPlane();
            float farPlane = cam->getFarClipPlane();
            int projType = static_cast<int>(cam->getProjectionType());
            const char* projItems[] = { "Perspective", "Orthographic" };
            if (ImGui::Combo("Projection", &projType, projItems, 2))
                cam->setProjectionType(static_cast<vroom::ProjectionType>(projType));
            if (cam->getProjectionType() == vroom::ProjectionType::Perspective) {
                if (ImGui::DragFloat("Field of View", &fov, 0.5f, 1.0f, 179.0f))
                    cam->setFieldOfView(fov);
            } else {
                float orthoSize = cam->getOrthographicSize();
                if (ImGui::DragFloat("Ortho Size", &orthoSize, 0.1f, 0.1f, 100.0f))
                    cam->setOrthographicSize(orthoSize);
            }
            if (ImGui::DragFloat("Near Clip", &nearPlane, 0.01f, 0.001f, farPlane - 0.001f))
                cam->setNearClipPlane(nearPlane);
            if (ImGui::DragFloat("Far Clip", &farPlane, 1.0f, nearPlane + 0.001f, 100000.0f))
                cam->setFarClipPlane(farPlane);
        }
        ImGui::End();
    });
#endif
    engine.run();
    return 0;
}
