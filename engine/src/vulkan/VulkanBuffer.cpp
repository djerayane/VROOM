#include "vroom/vulkan/VulkanBuffer.hpp"
#include "vroom/vulkan/VulkanDevice.hpp"
#include "vroom/logging/LogMacros.hpp"
#include <cstring>
#include <stdexcept>

namespace vroom {

VulkanBuffer::VulkanBuffer(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) : m_device(device), m_size(size) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device.getDevice(), &bufferInfo, nullptr, &m_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device.getDevice(), m_buffer, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, properties);
    if (vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &m_memory) != VK_SUCCESS) {
        vkDestroyBuffer(device.getDevice(), m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
        throw std::runtime_error("failed to allocate buffer memory!");
    }
    vkBindBufferMemory(device.getDevice(), m_buffer, m_memory, 0);
}

VulkanBuffer::~VulkanBuffer() {
    if (m_mapped) {
        unmap();
    }
    if (m_buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device.getDevice(), m_buffer, nullptr);
    }
    if (m_memory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device.getDevice(), m_memory, nullptr);
    }
}

void VulkanBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
    if (size == VK_WHOLE_SIZE) {
        size = m_size;
    }
    vkMapMemory(m_device.getDevice(), m_memory, offset, size, 0, &m_mapped);
}

void VulkanBuffer::unmap() {
    if (m_mapped) {
        vkUnmapMemory(m_device.getDevice(), m_memory);
        m_mapped = nullptr;
    }
}

void VulkanBuffer::writeToBuffer(void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (size == VK_WHOLE_SIZE) {
        size = m_size;
    }
    if (offset + size > m_size) {
        throw std::runtime_error("buffer overflow: size + offset exceeds buffer size!");
    }
    if (!m_mapped) {
        map(size, offset);
    }
    std::memcpy(m_mapped, data, (size_t)size);
}

std::unique_ptr<VulkanBuffer> VulkanBuffer::createVertexBuffer(VulkanDevice& device, const void* data, VkDeviceSize size) {
    auto buffer = std::make_unique<VulkanBuffer>(device, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    buffer->map();
    buffer->writeToBuffer(const_cast<void*>(data), size);
    buffer->unmap();
    return buffer;
}

std::unique_ptr<VulkanBuffer> VulkanBuffer::createIndexBuffer(VulkanDevice& device, const void* data, VkDeviceSize size) {
    auto buffer = std::make_unique<VulkanBuffer>(device, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    buffer->map();
    buffer->writeToBuffer(const_cast<void*>(data), size);
    buffer->unmap();
    return buffer;
}

} // namespace vroom
