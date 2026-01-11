#include <vroom/core/Engine.hpp>
#include <vroom/components/MeshRenderer.hpp>
#include <vroom/components/MeshFilter.hpp>
#include <vroom/asset/Mesh.hpp>
#include <vroom/asset/Material.hpp>
#include <vroom/asset/AssetManager.hpp>
#include <vroom/asset/AssetProvider.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <vroom/logging/LogMacros.hpp>

int main()
{
    // Use default config (headless=false)
    vroom::Engine engine;
    // Configure asset manager with disk provider
    engine.getAssetManager().addProvider(std::make_unique<vroom::DiskAssetProvider>("vroom_editor/assets"));
    // Create editor scene
    auto editorScene = std::make_shared<vroom::Scene>();
    editorScene->setSceneManager(&engine.getSceneManager());
    auto editorEntity = editorScene->createEntity(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f));
    auto &meshFilter = editorEntity->addComponent<vroom::MeshFilter>();
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
    auto &meshRenderer = editorEntity->addComponent<vroom::MeshRenderer>();
    auto defaultMaterial = std::make_shared<vroom::Material>();
    defaultMaterial->setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    meshRenderer.setMaterial(defaultMaterial);
    engine.getSceneManager().loadScene(editorScene);
    // Run engine
    engine.run();
    return 0;
}
