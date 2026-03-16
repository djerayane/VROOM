#pragma once

#include "vroom/core/Component.hpp"
#include "vroom/asset/Material.hpp"
#include <memory>

namespace vroom {

class MeshRenderer : public Component {
public:
    MeshRenderer() = default;
    ~MeshRenderer() override = default;

    void setMaterial(std::shared_ptr<Material> material);
    [[nodiscard]] std::shared_ptr<Material> getMaterial() const { return m_material; }

    // Override the new draw lifecycle hook
    void draw(VkCommandBuffer commandBuffer) override;

private:
    std::shared_ptr<Material> m_material;
};

} // namespace vroom

