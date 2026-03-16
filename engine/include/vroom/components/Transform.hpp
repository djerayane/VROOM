#pragma once

#include "vroom/core/Component.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vroom {

class Transform : public Component {
public:
    Transform(const glm::vec3& position = glm::vec3(0.0f),
                       const glm::vec3& rotation = glm::vec3(0.0f),
                       const glm::vec3& scale = glm::vec3(1.0f));
    ~Transform() override = default;

    /// \brief Gets the local position.
    const glm::vec3& getPosition() const { return m_position; }
    
    /// \brief Sets the local position.
    void setPosition(const glm::vec3& position);

    /// \brief Gets the local rotation in degrees (Euler angles).
    const glm::vec3& getRotation() const { return m_rotation; }
    
    /// \brief Sets the local rotation in degrees (Euler angles).
    void setRotation(const glm::vec3& rotation);

    /// \brief Gets the local scale.
    const glm::vec3& getScale() const { return m_scale; }
    
    /// \brief Sets the local scale.
    void setScale(const glm::vec3& scale);

    /// \brief Gets the local model matrix.
    /// Note: This transforms from local space to parent space.
    const glm::mat4& localToParentMatrix() const;

    /// \brief Gets the local to world matrix.
    /// Equivalent to Unity's localToWorldMatrix.
    glm::mat4 localToWorldMatrix() const;

    /// \brief Gets the world to local matrix.
    /// Equivalent to Unity's worldToLocalMatrix.
    glm::mat4 worldToLocalMatrix() const;

    // Direction vectors (in local space, affected by rotation)
    glm::vec3 getForward() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;

private:
    void updateLocalModelMatrix() const;

    glm::vec3 m_position{0.0f};
    glm::vec3 m_rotation{0.0f}; // Degrees
    glm::vec3 m_scale{1.0f};

    mutable glm::mat4 m_localModelMatrix{1.0f};
    mutable bool m_dirty = true;
};

} // namespace vroom

