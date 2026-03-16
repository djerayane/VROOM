#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "vroom/core/SceneManager.hpp"
#include "vroom/core/Component.hpp"

using namespace vroom;

class MockSceneManager : public SceneManager {
public:
    MOCK_METHOD(std::shared_ptr<Scene>, createSceneFromFile, (const std::string&), (override));
};

class SceneManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        sceneManager = std::make_shared<MockSceneManager>();
    }

    std::shared_ptr<MockSceneManager> sceneManager;
};

TEST_F(SceneManagerTest, InitialState) {
    // Default constructor creates one empty scene
    // SceneManagerTest constructs MockSceneManager which constructs SceneManager
    // SceneManager constructor calls make_shared<Scene> directly, doesn't call createSceneFromFile
    
    // We need to check the real SceneManager behavior via public API
    auto activeScene = sceneManager->getActiveScene();
    EXPECT_NE(activeScene, nullptr);
}

TEST_F(SceneManagerTest, LoadSceneReplacesActive) {
    auto scene1 = std::make_shared<Scene>();
    auto scene2 = std::make_shared<Scene>();
    
    EXPECT_CALL(*sceneManager, createSceneFromFile("scene1"))
        .WillOnce(testing::Return(scene1));
        
    sceneManager->loadScene("scene1");
    EXPECT_EQ(sceneManager->getActiveScene(), scene1);
    
    EXPECT_CALL(*sceneManager, createSceneFromFile("scene2"))
        .WillOnce(testing::Return(scene2));
        
    sceneManager->loadScene("scene2");
    EXPECT_EQ(sceneManager->getActiveScene(), scene2);
}

TEST_F(SceneManagerTest, LoadSceneAdditive) {
    auto scene1 = std::make_shared<Scene>();
    auto scene2 = std::make_shared<Scene>();
    
    EXPECT_CALL(*sceneManager, createSceneFromFile("scene1"))
        .WillOnce(testing::Return(scene1));
        
    sceneManager->loadScene("scene1");
    EXPECT_EQ(sceneManager->getActiveScene(), scene1);
    
    EXPECT_CALL(*sceneManager, createSceneFromFile("scene2"))
        .WillOnce(testing::Return(scene2));
        
    // Additive loading shouldn't change the active scene if one exists
    sceneManager->loadSceneAdditive("scene2");
    EXPECT_EQ(sceneManager->getActiveScene(), scene1);
}

TEST_F(SceneManagerTest, LoadSceneAsync) {
    auto scene1 = std::make_shared<Scene>();
    
    EXPECT_CALL(*sceneManager, createSceneFromFile("scene1"))
        .WillOnce(testing::Return(scene1));
        
    auto future = sceneManager->loadSceneAsync("scene1");
    future.wait();
    
    EXPECT_EQ(sceneManager->getActiveScene(), scene1);
}

TEST_F(SceneManagerTest, UnloadScene) {
    auto scene1 = std::make_shared<Scene>();
    auto scene2 = std::make_shared<Scene>();
    
    EXPECT_CALL(*sceneManager, createSceneFromFile("scene1"))
        .WillOnce(testing::Return(scene1));
    EXPECT_CALL(*sceneManager, createSceneFromFile("scene2"))
        .WillOnce(testing::Return(scene2));
        
    sceneManager->loadScene("scene1");
    sceneManager->loadSceneAdditive("scene2");
    
    // Current active is scene1
    EXPECT_EQ(sceneManager->getActiveScene(), scene1);
    
    sceneManager->unloadScene(scene1);
    
    // Should fall back to scene2
    EXPECT_EQ(sceneManager->getActiveScene(), scene2);
    
    sceneManager->unloadScene(scene2);
    EXPECT_EQ(sceneManager->getActiveScene(), nullptr);
}

class MockComponent : public Component {
public:
    MOCK_METHOD(void, update, (float), (override));
};

TEST_F(SceneManagerTest, UpdateCallsUpdateOnScenes) {
    auto scene1 = std::make_shared<Scene>();
    auto entity = scene1->createEntity();
    auto& component = entity->addComponent<MockComponent>();
    
    EXPECT_CALL(*sceneManager, createSceneFromFile("scene1"))
        .WillOnce(testing::Return(scene1));
        
    sceneManager->loadScene("scene1");
    
    EXPECT_CALL(component, update(0.1f)).Times(1);
    sceneManager->update(0.1f);
    
    // Unload to destroy scene and components
    sceneManager->unloadScene(scene1);

    // Let's clear the scene explicitly to force component destruction while we are still in the test body.
    scene1->clear();
}

TEST_F(SceneManagerTest, ComponentCanAccessSceneManager) {
    // The default scene created in SceneManager constructor should have the reference set
    auto activeScene = sceneManager->getActiveScene();
    ASSERT_NE(activeScene, nullptr);
    EXPECT_EQ(activeScene->getSceneManager(), sceneManager.get());

    auto entity = activeScene->createEntity();
    auto& component = entity->addComponent<MockComponent>();

    EXPECT_EQ(entity->getSceneManager(), sceneManager.get());
    EXPECT_EQ(component.getSceneManager(), sceneManager.get());
}
