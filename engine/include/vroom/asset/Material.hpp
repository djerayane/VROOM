#pragma once

#include "vroom/asset/Asset.hpp"
#include <glm/glm.hpp>

namespace vroom {

/**
 * @brief Asset representing a material.
 * Used by MeshRenderer to define how a mesh should be rendered.
 */
class Material : public Asset {
public:
    Material() = default;
    ~Material() override = default;

    void setColor(const glm::vec4& color) { m_color = color; }
    [[nodiscard]] const glm::vec4& getColor() const { return m_color; }

private:
    glm::vec4 m_color{1.0f, 1.0f, 1.0f, 1.0f};
};

} // namespace vroom

