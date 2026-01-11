#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "vroom/asset/Mesh.hpp"
#include "vroom/asset/ObjLoader.hpp"
#include "vroom/components/MeshFilter.hpp"
#include "vroom/components/MeshRenderer.hpp"
#include "vroom/asset/Material.hpp"
#include "vroom/core/Entity.hpp"
#include "vroom/core/Scene.hpp"

using namespace vroom;

// --- Mesh Asset Tests ---

TEST(MeshTest, CreateMesh) {
    std::vector<Vertex> vertices = {
        {{0, 0, 0}, {0, 1, 0}, {0, 0}},
        {{1, 0, 0}, {0, 1, 0}, {1, 0}},
        {{0, 1, 0}, {0, 1, 0}, {0, 1}}
    };
    std::vector<uint32_t> indices = {0, 1, 2};

    Mesh mesh(vertices, indices);

    EXPECT_EQ(mesh.getVertices().size(), 3);
    EXPECT_EQ(mesh.getIndices().size(), 3);
    EXPECT_EQ(mesh.getVertices()[1].position.x, 1.0f);
}

// --- ObjLoader Tests ---

TEST(ObjLoaderTest, LoadSimpleObj) {
    // Minimal OBJ content: a single triangle
    std::string objContent = 
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "vn 0.0 0.0 1.0\n"
        "vt 0.0 0.0\n"
        "vt 1.0 0.0\n"
        "vt 0.0 1.0\n"
        "f 1/1/1 2/2/1 3/3/1\n";

    std::vector<char> data(objContent.begin(), objContent.end());
    
    auto mesh = ObjLoader::load(data, "test.obj");

    ASSERT_NE(mesh, nullptr);
    EXPECT_EQ(mesh->getVertices().size(), 3);
    EXPECT_EQ(mesh->getIndices().size(), 3);
    
    const auto& v0 = mesh->getVertices()[0];
    EXPECT_FLOAT_EQ(v0.position.x, 0.0f);
    EXPECT_FLOAT_EQ(v0.position.y, 0.0f);
    EXPECT_FLOAT_EQ(v0.position.z, 0.0f);
    EXPECT_FLOAT_EQ(v0.normal.z, 1.0f);
}

TEST(ObjLoaderTest, LoadEmptyObj) {
    std::vector<char> data;
    auto mesh = ObjLoader::load(data, "empty.obj");
    
    ASSERT_NE(mesh, nullptr);
    EXPECT_TRUE(mesh->getVertices().empty());
    EXPECT_TRUE(mesh->getIndices().empty());
}

TEST(ObjLoaderTest, LoadInvalidIndices) {
    // OBJ referencing vertex index 5 when only 1 vertex exists
    std::string objContent = 
        "v 0.0 0.0 0.0\n"
        "f 5/5/5 5/5/5 5/5/5\n";

    std::vector<char> data(objContent.begin(), objContent.end());
    
    // This should not crash and should ideally handle the error gracefully
    // For now, we just want to ensure it doesn't crash.
    EXPECT_NO_THROW({
        auto mesh = ObjLoader::load(data, "invalid.obj");
        // We might expect an empty mesh or partial mesh depending on implementation
    });
}

// --- MeshFilter Tests ---

TEST(MeshFilterTest, SetAndGetMesh) {
    auto mesh = std::make_shared<Mesh>();
    MeshFilter meshFilter;
    
    EXPECT_EQ(meshFilter.getMesh(), nullptr);
    
    meshFilter.setMesh(mesh);
    EXPECT_EQ(meshFilter.getMesh(), mesh);
}

// --- MeshRenderer Tests ---

class MockMaterial : public Material {
};

TEST(MeshRendererTest, SetMaterial) {
    MeshRenderer renderer;
    auto material = std::make_shared<MockMaterial>();
    
    EXPECT_EQ(renderer.getMaterial(), nullptr);
    
    renderer.setMaterial(material);
    EXPECT_EQ(renderer.getMaterial(), material);
}

TEST(MeshRendererTest, Draw) {
    auto scene = std::make_shared<Scene>();
    auto entity = scene->createEntity();
    
    auto& meshFilter = entity->addComponent<MeshFilter>();
    auto& meshRenderer = entity->addComponent<MeshRenderer>();
    
    std::vector<Vertex> vertices(3);
    std::vector<uint32_t> indices = {0, 1, 2};
    auto mesh = std::make_shared<Mesh>(vertices, indices);
    
    meshFilter.setMesh(mesh);
    
    auto material = std::make_shared<Material>();
    meshRenderer.setMaterial(material);
    
    // We can't easily mock the internal drawing logic inside MeshRenderer::draw 
    // without dependency injection of a renderer backend, but we can at least ensure 
    // calling draw() doesn't crash and handles null checks.
    
    EXPECT_NO_THROW(meshRenderer.draw(nullptr));
    
    // Test with missing components/assets
    entity->setActive(false);
    EXPECT_NO_THROW(meshRenderer.draw(nullptr));
    
    entity->setActive(true);
    meshRenderer.setMaterial(nullptr);
    EXPECT_NO_THROW(meshRenderer.draw(nullptr));
}

// --- Scene Integration Tests ---

class MockRenderableComponent : public Component {
public:
    MOCK_METHOD(void, draw, (VkCommandBuffer), (override));
};

TEST(SceneDrawTest, PropagateDrawCall) {
    auto scene = std::make_shared<Scene>();
    auto entity = scene->createEntity();
    auto& component = entity->addComponent<MockRenderableComponent>();

    EXPECT_CALL(component, draw(testing::_)).Times(1);

    scene->draw(nullptr);
    scene->clear();
}

TEST(SceneDrawTest, DontDrawInactiveEntities) {
    auto scene = std::make_shared<Scene>();
    auto entity = scene->createEntity();
    auto& component = entity->addComponent<MockRenderableComponent>();
    
    entity->setActive(false);

    EXPECT_CALL(component, draw(testing::_)).Times(0);

    scene->draw(nullptr);
    scene->clear();
}

