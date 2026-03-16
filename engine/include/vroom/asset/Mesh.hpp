#pragma once

#include "vroom/asset/Asset.hpp"
#include <vector>
#include <glm/glm.hpp>

namespace vroom {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class Mesh : public Asset {
public:
    Mesh() = default;
    Mesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices)
        : m_vertices(std::move(vertices)), m_indices(std::move(indices)) {}
    
    ~Mesh() override = default;

    void setVertices(const std::vector<Vertex>& vertices) { m_vertices = vertices; }
    [[nodiscard]] const std::vector<Vertex>& getVertices() const { return m_vertices; }

    void setIndices(const std::vector<uint32_t>& indices) { m_indices = indices; }
    [[nodiscard]] const std::vector<uint32_t>& getIndices() const { return m_indices; }

    void setGPUData(std::shared_ptr<void> data) { m_gpuData = std::move(data); }
    [[nodiscard]] std::shared_ptr<void> getGPUData() const { return m_gpuData; }

private:
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::shared_ptr<void> m_gpuData;
};

} // namespace vroom

