#include <gtest/gtest.h>
#include <glm/gtc/epsilon.hpp>
#include "vroom/components/Transform.hpp"
#include "vroom/core/Entity.hpp"
#include "vroom/core/Scene.hpp"
#include "vroom/core/SceneManager.hpp"

using namespace vroom;

class TransformTest : public ::testing::Test {
protected:
    void SetUp() override {
        sceneManager = std::make_shared<SceneManager>();
        scene = std::make_shared<Scene>();
        scene->setSceneManager(sceneManager.get());
    }

    std::shared_ptr<SceneManager> sceneManager;
    std::shared_ptr<Scene> scene;

    bool vec3Eq(const glm::vec3& v1, const glm::vec3& v2, float epsilon = 0.0001f) {
        return glm::all(glm::epsilonEqual(v1, v2, epsilon));
    }

    bool mat4Eq(const glm::mat4& m1, const glm::mat4& m2, float epsilon = 0.0001f) {
        for (int i = 0; i < 4; ++i) {
            if (!glm::all(glm::epsilonEqual(m1[i], m2[i], epsilon))) return false;
        }
        return true;
    }
};

TEST_F(TransformTest, DefaultValues) {
    auto entity = scene->createEntity();
    auto* transform = entity->getComponent<Transform>();

    ASSERT_NE(transform, nullptr);
    EXPECT_TRUE(vec3Eq(transform->getPosition(), glm::vec3(0.0f)));
    EXPECT_TRUE(vec3Eq(transform->getRotation(), glm::vec3(0.0f)));
    EXPECT_TRUE(vec3Eq(transform->getScale(), glm::vec3(1.0f)));
}

TEST_F(TransformTest, CustomInitialValues) {
    glm::vec3 pos(1.0f, 2.0f, 3.0f);
    glm::vec3 rot(10.0f, 20.0f, 30.0f);
    glm::vec3 scale(0.5f, 0.5f, 0.5f);

    auto entity = scene->createEntity(pos, rot, scale);
    auto* transform = entity->getComponent<Transform>();

    ASSERT_NE(transform, nullptr);
    EXPECT_TRUE(vec3Eq(transform->getPosition(), pos));
    EXPECT_TRUE(vec3Eq(transform->getRotation(), rot));
    EXPECT_TRUE(vec3Eq(transform->getScale(), scale));
}

TEST_F(TransformTest, SettersAndGetters) {
    auto entity = scene->createEntity();
    auto* transform = entity->getComponent<Transform>();

    glm::vec3 newPos(5.0f, 5.0f, 5.0f);
    transform->setPosition(newPos);
    EXPECT_TRUE(vec3Eq(transform->getPosition(), newPos));

    glm::vec3 newRot(45.0f, 90.0f, 0.0f);
    transform->setRotation(newRot);
    EXPECT_TRUE(vec3Eq(transform->getRotation(), newRot));

    glm::vec3 newScale(2.0f, 2.0f, 2.0f);
    transform->setScale(newScale);
    EXPECT_TRUE(vec3Eq(transform->getScale(), newScale));
}

TEST_F(TransformTest, LocalToParentMatrix) {
    auto entity = scene->createEntity();
    auto* transform = entity->getComponent<Transform>();

    transform->setPosition(glm::vec3(1.0f, 2.0f, 3.0f));
    // No rotation or scale
    
    glm::mat4 expected(1.0f);
    expected = glm::translate(expected, glm::vec3(1.0f, 2.0f, 3.0f));

    EXPECT_TRUE(mat4Eq(transform->localToParentMatrix(), expected));
}

TEST_F(TransformTest, LocalToWorldMatrix_NoParent) {
    auto entity = scene->createEntity();
    auto* transform = entity->getComponent<Transform>();

    transform->setPosition(glm::vec3(10.0f, 0.0f, 0.0f));
    
    EXPECT_TRUE(mat4Eq(transform->localToWorldMatrix(), transform->localToParentMatrix()));
}

TEST_F(TransformTest, LocalToWorldMatrix_WithParent) {
    auto parent = scene->createEntity();
    auto child = scene->createEntity();
    
    parent->addChild(child.get());

    auto* parentTransform = parent->getComponent<Transform>();
    auto* childTransform = child->getComponent<Transform>();

    parentTransform->setPosition(glm::vec3(10.0f, 0.0f, 0.0f));
    childTransform->setPosition(glm::vec3(5.0f, 0.0f, 0.0f));

    glm::vec3 expectedWorldPos(15.0f, 0.0f, 0.0f);
    glm::mat4 worldMatrix = childTransform->localToWorldMatrix();
    glm::vec3 actualWorldPos = glm::vec3(worldMatrix[3]); // Extract translation column

    EXPECT_TRUE(vec3Eq(actualWorldPos, expectedWorldPos));
}

TEST_F(TransformTest, WorldToLocalMatrix) {
    auto entity = scene->createEntity();
    auto* transform = entity->getComponent<Transform>();

    transform->setPosition(glm::vec3(1.0f, 2.0f, 3.0f));
    
    glm::mat4 worldToLocal = transform->worldToLocalMatrix();
    glm::mat4 localToWorld = transform->localToWorldMatrix();

    EXPECT_TRUE(mat4Eq(worldToLocal * localToWorld, glm::mat4(1.0f)));
}

TEST_F(TransformTest, DirectionVectors) {
    auto entity = scene->createEntity();
    auto* transform = entity->getComponent<Transform>();

    // Default: Forward is -Z, Right is +X, Up is +Y
    EXPECT_TRUE(vec3Eq(transform->getForward(), glm::vec3(0.0f, 0.0f, -1.0f)));
    EXPECT_TRUE(vec3Eq(transform->getRight(), glm::vec3(1.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(vec3Eq(transform->getUp(), glm::vec3(0.0f, 1.0f, 0.0f)));

    // Rotate 90 degrees around Y axis
    transform->setRotation(glm::vec3(0.0f, 90.0f, 0.0f));
    
    // Forward should now be -X (rotated -Z 90 deg around Y) -> wait.
    // Initial Forward: (0, 0, -1). 
    // Rotate +90 around Y (0, 1, 0).
    // X becomes Z, Z becomes -X.
    // So (0, 0, -1) -> (-1, 0, 0).
    EXPECT_TRUE(vec3Eq(transform->getForward(), glm::vec3(-1.0f, 0.0f, 0.0f)));
    
    // Right was +X (1, 0, 0).
    // Rotated +90 around Y.
    // X becomes -Z.
    // So (1, 0, 0) -> (0, 0, -1).
    EXPECT_TRUE(vec3Eq(transform->getRight(), glm::vec3(0.0f, 0.0f, -1.0f)));

    // Up should remain +Y
    EXPECT_TRUE(vec3Eq(transform->getUp(), glm::vec3(0.0f, 1.0f, 0.0f)));
}
