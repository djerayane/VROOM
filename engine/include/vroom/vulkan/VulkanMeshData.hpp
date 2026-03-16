#pragma once

#include <memory>

namespace vroom {

class VulkanBuffer;

struct VulkanMeshData {
    std::unique_ptr<VulkanBuffer> vertexBuffer;
    std::unique_ptr<VulkanBuffer> indexBuffer;
};

} // namespace vroom
