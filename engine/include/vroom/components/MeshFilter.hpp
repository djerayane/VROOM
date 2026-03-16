#pragma once

#include "vroom/core/Component.hpp"
#include "vroom/asset/Mesh.hpp"
#include <memory>

namespace vroom {

class MeshFilter : public Component {
public:
    MeshFilter() = default;
    ~MeshFilter() override = default;

    void setMesh(std::shared_ptr<Mesh> mesh);
    [[nodiscard]] std::shared_ptr<Mesh> getMesh() const { return m_mesh; }

private:
    std::shared_ptr<Mesh> m_mesh;
};

} // namespace vroom
