#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "vroom/core/Scene.hpp"
#include "vroom/core/Entity.hpp"
#include "vroom/core/Component.hpp"

// Define a dummy struct for VkCommandBuffer if not available, 
// or just use the forward declaration from the headers.
// Since we include Scene.hpp which includes Component.hpp, 
// Component.hpp has `using VkCommandBuffer = struct VkCommandBuffer_T*;`
// We can just cast an integer to this type for testing.

using VkCommandBuffer = struct VkCommandBuffer_T*;

class MockDrawComponent : public vroom::Component {
public:
    MOCK_METHOD(void, draw, (VkCommandBuffer), (override));
};

class DrawingTest : public ::testing::Test {
protected:
    void SetUp() override {
        scene = std::make_shared<vroom::Scene>();
    }

    std::shared_ptr<vroom::Scene> scene;
};

TEST_F(DrawingTest, DrawPropagatesToComponents) {
    auto entity = scene->createEntity();
    auto& mockComponent = entity->addComponent<MockDrawComponent>();

    // Create a dummy command buffer pointer
    VkCommandBuffer dummyBuffer = reinterpret_cast<VkCommandBuffer>(0x12345678);

    // Expect draw to be called with the specific command buffer
    EXPECT_CALL(mockComponent, draw(dummyBuffer)).Times(1);

    scene->draw(dummyBuffer);
}

TEST_F(DrawingTest, DrawSkipsInactiveEntities) {
    auto entity = scene->createEntity();
    auto& mockComponent = entity->addComponent<MockDrawComponent>();
    
    entity->setActive(false);

    VkCommandBuffer dummyBuffer = reinterpret_cast<VkCommandBuffer>(0x12345678);

    // Expect draw NOT to be called
    EXPECT_CALL(mockComponent, draw(testing::_)).Times(0);

    scene->draw(dummyBuffer);
}

TEST_F(DrawingTest, DrawSkipsDisabledComponents) {
    auto entity = scene->createEntity();
    auto& mockComponent = entity->addComponent<MockDrawComponent>();
    
    mockComponent.setEnabled(false);

    VkCommandBuffer dummyBuffer = reinterpret_cast<VkCommandBuffer>(0x12345678);

    // Expect draw NOT to be called
    EXPECT_CALL(mockComponent, draw(testing::_)).Times(0);

    scene->draw(dummyBuffer);
}

TEST_F(DrawingTest, DrawPropagatesToChildren) {
    auto parent = scene->createEntity();
    auto child = scene->createEntity();
    parent->addChild(child.get()); // Note: addChild takes raw pointer, child is shared_ptr in scene

    auto& mockComponent = child->addComponent<MockDrawComponent>();

    VkCommandBuffer dummyBuffer = reinterpret_cast<VkCommandBuffer>(0x12345678);

    // Expect draw to be called
    EXPECT_CALL(mockComponent, draw(dummyBuffer)).Times(1);

    scene->draw(dummyBuffer);
}


