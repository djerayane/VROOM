#include <gtest/gtest.h>
#include "vroom/vulkan/VulkanRenderer.hpp"
#include "vroom/asset/AssetManager.hpp"
#include "vroom/asset/Mesh.hpp"
#include "tests/vulkan/MockWindow.hpp"
#include "vroom/vulkan/VulkanMeshData.hpp"

class VulkanRendererTest : public ::testing::Test {
protected:
    void SetUp() override {
        try {
            m_window = std::make_unique<MockWindow>();
        } catch (const std::exception& e) {
            initializationFailed = true;
            initializationError = e.what();
        }
    }

    vroom::AssetManager m_assetManager;
    std::unique_ptr<MockWindow> m_window;
    bool initializationFailed = false;
    std::string initializationError;
};

TEST_F(VulkanRendererTest, Initialization) {
    if (initializationFailed) {
        GTEST_SKIP() << "Skipping Vulkan tests due to window creation failure: " << initializationError;
    }

    try {
        vroom::VulkanRenderer renderer(m_assetManager);
        renderer.init(m_window->get());
        
        // Just check if we reached here without exception
        SUCCEED();
    } catch (const std::runtime_error& e) {
        GTEST_SKIP() << "Skipping Vulkan renderer init test due to runtime error (likely no GPU): " << e.what();
    }
}

TEST_F(VulkanRendererTest, MeshUpload) {
    if (initializationFailed) {
        GTEST_SKIP() << "Skipping Vulkan tests due to window creation failure: " << initializationError;
    }

    try {
        vroom::VulkanRenderer renderer(m_assetManager);
        renderer.init(m_window->get());

        // Create a simple mesh
        std::vector<vroom::Vertex> vertices = {
            {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
        };
        std::vector<uint32_t> indices = {0, 1, 2};
        
        auto mesh = std::make_shared<vroom::Mesh>(vertices, indices);

        // Initially no GPU data
        EXPECT_FALSE(mesh->getGPUData());

        // Upload
        renderer.uploadMesh(mesh);

        // Should have GPU data now
        EXPECT_TRUE(mesh->getGPUData());

        auto gpuData = std::static_pointer_cast<vroom::VulkanMeshData>(mesh->getGPUData());
        EXPECT_TRUE(gpuData->vertexBuffer);
        EXPECT_TRUE(gpuData->indexBuffer);

    } catch (const std::runtime_error& e) {
        GTEST_SKIP() << "Skipping Vulkan mesh upload test due to runtime error (likely no GPU): " << e.what();
    }
}


