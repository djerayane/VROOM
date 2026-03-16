#pragma once

#include "vroom/core/Component.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace vroom {

/// \brief Projection type for the camera.
enum class ProjectionType {
    Perspective,
    Orthographic
};

/// \brief Normalized viewport rectangle (values in [0,1] range).
struct ViewportRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
};

/// \brief Camera component that defines how the scene is rendered.
///
/// Works in tandem with the Transform component to compute view and projection matrices.
/// Modeled after Unity's Camera component. Supports perspective and orthographic projection,
/// viewport rectangles, clear color, and depth-based ordering for multiple cameras.
/// The targetTexture field is reserved for future RenderTexture support.
class Camera : public Component {
public:
    Camera() = default;
    ~Camera() override = default;

    // -- Projection type --

    /// \brief Gets the projection type.
    [[nodiscard]] ProjectionType getProjectionType() const { return m_projectionType; }

    /// \brief Sets the projection type.
    void setProjectionType(ProjectionType type) { m_projectionType = type; }

    // -- Field of view (perspective only) --

    /// \brief Gets the vertical field of view in degrees.
    [[nodiscard]] float getFieldOfView() const { return m_fieldOfView; }

    /// \brief Sets the vertical field of view in degrees.
    void setFieldOfView(float fov) { m_fieldOfView = fov; }

    // -- Orthographic size --

    /// \brief Gets the orthographic half-height.
    [[nodiscard]] float getOrthographicSize() const { return m_orthographicSize; }

    /// \brief Sets the orthographic half-height.
    void setOrthographicSize(float size) { m_orthographicSize = size; }

    // -- Clip planes --

    /// \brief Gets the near clip plane distance.
    [[nodiscard]] float getNearClipPlane() const { return m_nearClipPlane; }

    /// \brief Sets the near clip plane distance.
    void setNearClipPlane(float near) { m_nearClipPlane = near; }

    /// \brief Gets the far clip plane distance.
    [[nodiscard]] float getFarClipPlane() const { return m_farClipPlane; }

    /// \brief Sets the far clip plane distance.
    void setFarClipPlane(float far) { m_farClipPlane = far; }

    // -- Background color --

    /// \brief Gets the background (clear) color.
    [[nodiscard]] const glm::vec4& getBackgroundColor() const { return m_backgroundColor; }

    /// \brief Sets the background (clear) color.
    void setBackgroundColor(const glm::vec4& color) { m_backgroundColor = color; }

    // -- Depth (rendering priority) --

    /// \brief Gets the camera depth (rendering priority). Lower renders first.
    [[nodiscard]] int getDepth() const { return m_depth; }

    /// \brief Sets the camera depth (rendering priority).
    void setDepth(int depth) { m_depth = depth; }

    // -- Viewport rect --

    /// \brief Gets the normalized viewport rectangle.
    [[nodiscard]] const ViewportRect& getViewportRect() const { return m_viewportRect; }

    /// \brief Sets the normalized viewport rectangle.
    void setViewportRect(const ViewportRect& rect) { m_viewportRect = rect; }

    // -- Target texture (placeholder for future RenderTexture) --

    /// \brief Gets the render target texture. nullptr means render to screen.
    [[nodiscard]] std::shared_ptr<void> getTargetTexture() const { return m_targetTexture; }

    /// \brief Sets the render target texture. Pass nullptr to render to screen.
    void setTargetTexture(std::shared_ptr<void> texture) { m_targetTexture = std::move(texture); }

    /// \brief Returns true if this camera renders to screen (no target texture).
    [[nodiscard]] bool rendersToScreen() const { return m_targetTexture == nullptr; }

    // -- Matrix computation --

    /// \brief Computes the view matrix from the entity's Transform.
    /// \return The view matrix (world-to-camera transform).
    [[nodiscard]] glm::mat4 getViewMatrix() const;

    /// \brief Computes the projection matrix.
    /// \param aspectRatio The viewport aspect ratio (width / height).
    /// \return The projection matrix.
    [[nodiscard]] glm::mat4 getProjectionMatrix(float aspectRatio) const;

    /// \brief Computes the combined view-projection matrix.
    /// \param aspectRatio The viewport aspect ratio (width / height).
    /// \return The view-projection matrix (projection * view).
    [[nodiscard]] glm::mat4 getViewProjectionMatrix(float aspectRatio) const;

    // -- Static helpers --

    /// \brief Finds the main camera in the given scene.
    /// Returns the enabled Camera with the lowest depth value,
    /// or nullptr if no camera exists.
    /// \param scene The scene to search.
    /// \return Pointer to the main camera, or nullptr.
    static Camera* getMain(class Scene& scene);

private:
    ProjectionType m_projectionType = ProjectionType::Perspective;
    float m_fieldOfView = 60.0f;
    float m_orthographicSize = 5.0f;
    float m_nearClipPlane = 0.1f;
    float m_farClipPlane = 1000.0f;
    glm::vec4 m_backgroundColor{0.192f, 0.302f, 0.475f, 1.0f};
    int m_depth = 0;
    ViewportRect m_viewportRect{};
    std::shared_ptr<void> m_targetTexture = nullptr;
};

} // namespace vroom
