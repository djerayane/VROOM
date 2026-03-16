#include "vroom/components/Camera.hpp"
#include "vroom/components/Transform.hpp"
#include "vroom/core/Entity.hpp"
#include "vroom/core/Scene.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace vroom {

static void findCameraRecursive(Entity* entity, Camera*& best) {
    if (!entity || !entity->isActive()) return;
    auto* cam = entity->getComponent<Camera>();
    if (cam && cam->isEnabled()) {
        if (!best || cam->getDepth() < best->getDepth()) {
            best = cam;
        }
    }
    for (auto* child : entity->getChildren()) {
        findCameraRecursive(child, best);
    }
}

glm::mat4 Camera::getViewMatrix() const {
    if (!m_entity) {
        return glm::mat4(1.0f);
    }
    auto* transform = m_entity->getComponent<Transform>();
    if (!transform) {
        return glm::mat4(1.0f);
    }
    // The view matrix is the inverse of the camera's world transform.
    return transform->worldToLocalMatrix();
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    if (m_projectionType == ProjectionType::Perspective) {
        // perspectiveRH_ZO maps near→0, far→1 matching Vulkan's [0,1] depth range.
        // Y is flipped to convert from OpenGL's Y-up NDC to Vulkan's Y-down NDC.
        glm::mat4 proj = glm::perspectiveRH_ZO(
            glm::radians(m_fieldOfView),
            aspectRatio,
            m_nearClipPlane,
            m_farClipPlane
        );
        proj[1][1] *= -1.0f;
        return proj;
    }
    float halfHeight = m_orthographicSize;
    float halfWidth = halfHeight * aspectRatio;
    glm::mat4 proj = glm::orthoRH_ZO(
        -halfWidth, halfWidth,
        -halfHeight, halfHeight,
        m_nearClipPlane, m_farClipPlane
    );
    proj[1][1] *= -1.0f;
    return proj;
}

glm::mat4 Camera::getViewProjectionMatrix(float aspectRatio) const {
    return getProjectionMatrix(aspectRatio) * getViewMatrix();
}

Camera* Camera::getMain(Scene& scene) {
    Camera* best = nullptr;
    for (auto* root : scene.getRootEntities()) {
        findCameraRecursive(root, best);
    }
    return best;
}

} // namespace vroom
