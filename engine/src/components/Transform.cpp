#include "vroom/components/Transform.hpp"
#include "vroom/core/Entity.hpp"

namespace vroom {

Transform::Transform(const glm::vec3& position,
                                       const glm::vec3& rotation,
                                       const glm::vec3& scale)
    : m_position(position), m_rotation(rotation), m_scale(scale) {
}

void Transform::setPosition(const glm::vec3& position) {
    m_position = position;
    m_dirty = true;
}

void Transform::setRotation(const glm::vec3& rotation) {
    m_rotation = rotation;
    m_dirty = true;
}

void Transform::setScale(const glm::vec3& scale) {
    m_scale = scale;
    m_dirty = true;
}

const glm::mat4& Transform::localToParentMatrix() const {
    if (m_dirty) {
        updateLocalModelMatrix();
    }
    return m_localModelMatrix;
}

glm::mat4 Transform::localToWorldMatrix() const {
    const auto& localMatrix = localToParentMatrix();
    
    if (m_entity && m_entity->getParent()) {
        auto* parentTransform = m_entity->getParent()->getComponent<Transform>();
        if (parentTransform) {
            return parentTransform->localToWorldMatrix() * localMatrix;
        }
    }
    
    return localMatrix;
}

glm::mat4 Transform::worldToLocalMatrix() const {
    return glm::inverse(localToWorldMatrix());
}

void Transform::updateLocalModelMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    
    model = glm::translate(model, m_position);
    
    // Apply rotation (Y, X, Z order is one convention, but let's do X, Y, Z)
    // Note: standard Euler angles application depends on convention. 
    // GLM rotate rotates around an axis. 
    // We'll apply rotations in X, Y, Z order.
    model = glm::rotate(model, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    
    model = glm::scale(model, m_scale);
    
    m_localModelMatrix = model;
    m_dirty = false;
}

glm::vec3 Transform::getForward() const {
    // Assuming -Z is forward in our coordinate system (common in OpenGL/Vulkan)
    // But check engine conventions. 
    // Standard OpenGL: -Z forward, +Y up, +X right.
    
    glm::vec3 forward;
    float pitch = glm::radians(m_rotation.x);
    float yaw = glm::radians(m_rotation.y);
    // float roll = glm::radians(m_rotation.z);

    // Calculate forward vector from Euler angles
    forward.x = cos(pitch) * sin(yaw);
    forward.y = sin(pitch);
    forward.z = cos(pitch) * cos(yaw);
    
    // If using -Z as forward (and initial rotation 0,0,0 faces -Z):
    // With 0,0,0: cos(0)*sin(0)=0, sin(0)=0, cos(0)*cos(0)=1 => (0,0,1). This is +Z.
    // So we need to adjust if we want -Z.
    
    // A robust way is to transform the forward vector (0, 0, -1) by the rotation matrix.
    // But simpler: use the rotation matrix we already calculate.
    
    // However, we don't store rotation matrix separately, it's in model matrix (with scale).
    // If we extract rotation from model matrix, we have to be careful about scale.
    
    // Let's construct a pure rotation matrix for direction vectors.
    glm::mat4 rotationMat(1.0f);
    rotationMat = glm::rotate(rotationMat, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotationMat = glm::rotate(rotationMat, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationMat = glm::rotate(rotationMat, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Assuming forward is -Z
    return glm::normalize(glm::vec3(rotationMat * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f)));
}

glm::vec3 Transform::getRight() const {
    glm::mat4 rotationMat(1.0f);
    rotationMat = glm::rotate(rotationMat, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotationMat = glm::rotate(rotationMat, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationMat = glm::rotate(rotationMat, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Right is +X
    return glm::normalize(glm::vec3(rotationMat * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
}

glm::vec3 Transform::getUp() const {
    glm::mat4 rotationMat(1.0f);
    rotationMat = glm::rotate(rotationMat, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotationMat = glm::rotate(rotationMat, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotationMat = glm::rotate(rotationMat, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    
    // Up is +Y
    return glm::normalize(glm::vec3(rotationMat * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
}

} // namespace vroom

