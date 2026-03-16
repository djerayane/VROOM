#include <gtest/gtest.h>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include "vroom/components/Camera.hpp"
#include "vroom/components/Transform.hpp"
#include "vroom/core/Entity.hpp"
#include "vroom/core/Scene.hpp"
#include "vroom/core/SceneManager.hpp"

using namespace vroom;

class CameraTest : public ::testing::Test {
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

// -- Default values --

TEST_F(CameraTest, DefaultProjectionType) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    EXPECT_EQ(cam.getProjectionType(), ProjectionType::Perspective);
}

TEST_F(CameraTest, DefaultFieldOfView) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    EXPECT_FLOAT_EQ(cam.getFieldOfView(), 60.0f);
}

TEST_F(CameraTest, DefaultClipPlanes) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    EXPECT_FLOAT_EQ(cam.getNearClipPlane(), 0.1f);
    EXPECT_FLOAT_EQ(cam.getFarClipPlane(), 1000.0f);
}

TEST_F(CameraTest, DefaultOrthographicSize) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    EXPECT_FLOAT_EQ(cam.getOrthographicSize(), 5.0f);
}

TEST_F(CameraTest, DefaultDepth) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    EXPECT_EQ(cam.getDepth(), 0);
}

TEST_F(CameraTest, DefaultViewportRect) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    const auto& rect = cam.getViewportRect();
    EXPECT_FLOAT_EQ(rect.x, 0.0f);
    EXPECT_FLOAT_EQ(rect.y, 0.0f);
    EXPECT_FLOAT_EQ(rect.width, 1.0f);
    EXPECT_FLOAT_EQ(rect.height, 1.0f);
}

TEST_F(CameraTest, DefaultRendersToScreen) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    EXPECT_TRUE(cam.rendersToScreen());
    EXPECT_EQ(cam.getTargetTexture(), nullptr);
}

// -- Setters --

TEST_F(CameraTest, SetProjectionType) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    cam.setProjectionType(ProjectionType::Orthographic);
    EXPECT_EQ(cam.getProjectionType(), ProjectionType::Orthographic);
}

TEST_F(CameraTest, SetFieldOfView) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    cam.setFieldOfView(90.0f);
    EXPECT_FLOAT_EQ(cam.getFieldOfView(), 90.0f);
}

TEST_F(CameraTest, SetClipPlanes) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    cam.setNearClipPlane(0.5f);
    cam.setFarClipPlane(500.0f);
    EXPECT_FLOAT_EQ(cam.getNearClipPlane(), 0.5f);
    EXPECT_FLOAT_EQ(cam.getFarClipPlane(), 500.0f);
}

TEST_F(CameraTest, SetOrthographicSize) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    cam.setOrthographicSize(10.0f);
    EXPECT_FLOAT_EQ(cam.getOrthographicSize(), 10.0f);
}

TEST_F(CameraTest, SetBackgroundColor) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    glm::vec4 color(1.0f, 0.0f, 0.0f, 1.0f);
    cam.setBackgroundColor(color);
    EXPECT_EQ(cam.getBackgroundColor(), color);
}

TEST_F(CameraTest, SetDepth) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    cam.setDepth(5);
    EXPECT_EQ(cam.getDepth(), 5);
}

TEST_F(CameraTest, SetViewportRect) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    ViewportRect rect{0.0f, 0.0f, 0.5f, 0.5f};
    cam.setViewportRect(rect);
    const auto& r = cam.getViewportRect();
    EXPECT_FLOAT_EQ(r.x, 0.0f);
    EXPECT_FLOAT_EQ(r.width, 0.5f);
}

TEST_F(CameraTest, SetTargetTexture) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    auto fakeTexture = std::make_shared<int>(42);
    cam.setTargetTexture(fakeTexture);
    EXPECT_FALSE(cam.rendersToScreen());
    EXPECT_NE(cam.getTargetTexture(), nullptr);
    cam.setTargetTexture(nullptr);
    EXPECT_TRUE(cam.rendersToScreen());
}

// -- View matrix --

TEST_F(CameraTest, ViewMatrix_IdentityAtOrigin) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    // Camera at origin with no rotation should produce identity view.
    EXPECT_TRUE(mat4Eq(cam.getViewMatrix(), glm::mat4(1.0f)));
}

TEST_F(CameraTest, ViewMatrix_TranslatedCamera) {
    auto entity = scene->createEntity(glm::vec3(0.0f, 0.0f, 5.0f));
    auto& cam = entity->addComponent<Camera>();
    glm::mat4 view = cam.getViewMatrix();
    // A camera at (0,0,5) should translate world by (0,0,-5).
    glm::vec4 worldOrigin = view * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_TRUE(vec3Eq(glm::vec3(worldOrigin), glm::vec3(0.0f, 0.0f, -5.0f)));
}

TEST_F(CameraTest, ViewMatrix_IsInverseOfWorldTransform) {
    auto entity = scene->createEntity(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(10.0f, 20.0f, 0.0f));
    auto& cam = entity->addComponent<Camera>();
    auto* transform = entity->getComponent<Transform>();
    glm::mat4 view = cam.getViewMatrix();
    glm::mat4 world = transform->localToWorldMatrix();
    EXPECT_TRUE(mat4Eq(view * world, glm::mat4(1.0f)));
}

// -- Projection matrix --

TEST_F(CameraTest, PerspectiveProjection_ValidMatrix) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    cam.setFieldOfView(60.0f);
    cam.setNearClipPlane(0.1f);
    cam.setFarClipPlane(100.0f);
    float aspect = 16.0f / 9.0f;
    glm::mat4 proj = cam.getProjectionMatrix(aspect);
    glm::mat4 expected = glm::perspectiveRH_ZO(glm::radians(60.0f), aspect, 0.1f, 100.0f);
    expected[1][1] *= -1.0f;
    EXPECT_TRUE(mat4Eq(proj, expected));
}

TEST_F(CameraTest, OrthographicProjection_ValidMatrix) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    cam.setProjectionType(ProjectionType::Orthographic);
    cam.setOrthographicSize(5.0f);
    cam.setNearClipPlane(0.1f);
    cam.setFarClipPlane(100.0f);
    float aspect = 16.0f / 9.0f;
    glm::mat4 proj = cam.getProjectionMatrix(aspect);
    // Orthographic: expected from glm::ortho with Y flipped.
    float halfW = 5.0f * aspect;
    glm::mat4 expected = glm::orthoRH_ZO(-halfW, halfW, -5.0f, 5.0f, 0.1f, 100.0f);
    expected[1][1] *= -1.0f;
    EXPECT_TRUE(mat4Eq(proj, expected));
}

// -- View-projection --

TEST_F(CameraTest, ViewProjectionMatrix_IsProjTimesView) {
    auto entity = scene->createEntity(glm::vec3(0.0f, 5.0f, 10.0f), glm::vec3(30.0f, 0.0f, 0.0f));
    auto& cam = entity->addComponent<Camera>();
    float aspect = 1.5f;
    glm::mat4 vp = cam.getViewProjectionMatrix(aspect);
    glm::mat4 expected = cam.getProjectionMatrix(aspect) * cam.getViewMatrix();
    EXPECT_TRUE(mat4Eq(vp, expected));
}

// -- getMain --

TEST_F(CameraTest, GetMain_ReturnsNullWhenNoCamera) {
    scene->createEntity();
    EXPECT_EQ(Camera::getMain(*scene), nullptr);
}

TEST_F(CameraTest, GetMain_ReturnsSingleCamera) {
    auto entity = scene->createEntity();
    auto& cam = entity->addComponent<Camera>();
    EXPECT_EQ(Camera::getMain(*scene), &cam);
}

TEST_F(CameraTest, GetMain_ReturnsLowestDepth) {
    auto e1 = scene->createEntity();
    auto& cam1 = e1->addComponent<Camera>();
    cam1.setDepth(10);
    auto e2 = scene->createEntity();
    auto& cam2 = e2->addComponent<Camera>();
    cam2.setDepth(1);
    auto e3 = scene->createEntity();
    auto& cam3 = e3->addComponent<Camera>();
    cam3.setDepth(5);
    EXPECT_EQ(Camera::getMain(*scene), &cam2);
}

TEST_F(CameraTest, GetMain_SkipsDisabledCamera) {
    auto e1 = scene->createEntity();
    auto& cam1 = e1->addComponent<Camera>();
    cam1.setDepth(0);
    cam1.setEnabled(false);
    auto e2 = scene->createEntity();
    auto& cam2 = e2->addComponent<Camera>();
    cam2.setDepth(5);
    EXPECT_EQ(Camera::getMain(*scene), &cam2);
}

TEST_F(CameraTest, GetMain_SkipsInactiveEntity) {
    auto e1 = scene->createEntity();
    auto& cam1 = e1->addComponent<Camera>();
    cam1.setDepth(0);
    e1->setActive(false);
    auto e2 = scene->createEntity();
    auto& cam2 = e2->addComponent<Camera>();
    cam2.setDepth(5);
    EXPECT_EQ(Camera::getMain(*scene), &cam2);
}

TEST_F(CameraTest, GetMain_FindsCameraInChildEntity) {
    auto parent = scene->createEntity();
    auto child = scene->createEntity();
    parent->addChild(child.get());
    auto& cam = child->addComponent<Camera>();
    EXPECT_EQ(Camera::getMain(*scene), &cam);
}
