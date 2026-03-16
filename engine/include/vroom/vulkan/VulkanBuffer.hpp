#pragma once

#include <vulkan/vulkan.h>
#include <memory>

namespace vroom {

class VulkanDevice;

class VulkanBuffer {
public:
    VulkanBuffer(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~VulkanBuffer();
    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    [[nodiscard]] VkBuffer getBuffer() const { return m_buffer; }
    [[nodiscard]] VkDeviceMemory getMemory() const { return m_memory; }
    [[nodiscard]] VkDeviceSize getSize() const { return m_size; }
    void map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void unmap();
    void writeToBuffer(void* data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    static std::unique_ptr<VulkanBuffer> createVertexBuffer(VulkanDevice& device, const void* data, VkDeviceSize size);
    static std::unique_ptr<VulkanBuffer> createIndexBuffer(VulkanDevice& device, const void* data, VkDeviceSize size);

private:
    VulkanDevice& m_device;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkDeviceSize m_size = 0;
    void* m_mapped = nullptr;
};

} // namespace vroom
