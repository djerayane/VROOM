#include "vroom/components/MeshFilter.hpp"

namespace vroom {

void MeshFilter::setMesh(std::shared_ptr<Mesh> mesh) {
    m_mesh = std::move(mesh);
}

} // namespace vroom
