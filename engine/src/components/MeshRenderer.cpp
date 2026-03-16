#include "vroom/components/MeshRenderer.hpp"
#include "vroom/components/MeshFilter.hpp"
#include "vroom/components/Transform.hpp"
#include "vroom/core/Entity.hpp"
#include "vroom/core/Engine.hpp"
#include "vroom/asset/Mesh.hpp"
#include "vroom/logging/LogMacros.hpp"
#include "vroom/vulkan/VulkanMeshData.hpp"
#include "vroom/vulkan/VulkanRenderer.hpp"
#include "vroom/vulkan/VulkanBuffer.hpp"

namespace vroom {

void MeshRenderer::setMaterial(std::shared_ptr<Material> material) {
    m_material = std::move(material);
}

static void ensureMeshUploaded(std::shared_ptr<Mesh> mesh) {
    if (!mesh->getGPUData()) {
        auto* engine = Engine::tryGetInstance();
        auto renderer = engine ? engine->getRenderer() : nullptr;
        if (renderer) {
            renderer->uploadMesh(mesh);
        }
    }
}

static void bindMeshBuffers(VkCommandBuffer commandBuffer, const std::shared_ptr<VulkanMeshData>& gpuData) {
    VkBuffer vertexBuffers[] = { gpuData->vertexBuffer->getBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(reinterpret_cast<::VkCommandBuffer>(commandBuffer), 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(reinterpret_cast<::VkCommandBuffer>(commandBuffer), gpuData->indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

static void pushModelMatrix(VkCommandBuffer commandBuffer, const glm::mat4& model) {
    auto* engine = Engine::tryGetInstance();
    auto renderer = engine ? engine->getRenderer() : nullptr;
    if (renderer) {
        VkPipelineLayout layout = renderer->getPipelineLayout();
        // Model matrix sits after the viewProjection matrix in push constants.
        constexpr uint32_t MODEL_OFFSET = sizeof(glm::mat4);
        vkCmdPushConstants(reinterpret_cast<::VkCommandBuffer>(commandBuffer), layout, VK_SHADER_STAGE_VERTEX_BIT, MODEL_OFFSET, sizeof(glm::mat4), &model);
    }
}

void MeshRenderer::draw(VkCommandBuffer commandBuffer) {
    if (!m_enabled || !m_entity) return;
    auto* meshFilter = m_entity->getComponent<MeshFilter>();
    if (!meshFilter) return;
    auto mesh = meshFilter->getMesh();
    if (!mesh) return;
    ensureMeshUploaded(mesh);
    auto gpuData = std::static_pointer_cast<VulkanMeshData>(mesh->getGPUData());
    if (!gpuData || !gpuData->vertexBuffer || !gpuData->indexBuffer) return;
    bindMeshBuffers(commandBuffer, gpuData);
    auto transform = m_entity->getComponent<Transform>();
    if (transform) {
        pushModelMatrix(commandBuffer, transform->localToWorldMatrix());
    }
    vkCmdDrawIndexed(reinterpret_cast<::VkCommandBuffer>(commandBuffer), static_cast<uint32_t>(mesh->getIndices().size()), 1, 0, 0, 0);
}

} // namespace vroom
