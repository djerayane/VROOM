#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <string>

#include "vroom/vulkan/VulkanDevice.hpp"
#include "vroom/vulkan/VulkanSwapChain.hpp"
#include "vroom/asset/AssetManager.hpp" // Include AssetManager

struct GLFWwindow;

namespace vroom {

class Mesh;
class Scene;

class VulkanRenderer {
public:
    VulkanRenderer(AssetManager& assetManager); // Pass AssetManager
    ~VulkanRenderer();

    void init(GLFWwindow* window);
    void drawFrame(std::shared_ptr<Scene> scene);
    void deviceWaitIdle();
    void setFramebufferResized(bool resized) { m_framebufferResized = resized; }
    
    void uploadMesh(std::shared_ptr<Mesh> mesh);
    
    VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }
    VulkanDevice& getDevice() { return *m_device; }

private:
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandBuffers();
    void createSyncObjects();
    void recreateSwapChain();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, std::shared_ptr<Scene> scene);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    // Removed readFile as we use AssetManager now

    AssetManager& m_assetManager; // Reference to AssetManager
    VkPipelineVertexInputStateCreateInfo createVertexInputState(VkVertexInputBindingDescription& bindingDesc, std::array<VkVertexInputAttributeDescription, 3>& attrDescs);
    VkPipelineLayoutCreateInfo createPipelineLayoutInfo(VkPushConstantRange& pushConstantRange);
    VkGraphicsPipelineCreateInfo createGraphicsPipelineInfo(VkPipelineShaderStageCreateInfo* shaderStages, VkPipelineVertexInputStateCreateInfo& vertexInput, VkPipelineInputAssemblyStateCreateInfo& inputAssembly, VkPipelineViewportStateCreateInfo& viewportState, VkPipelineRasterizationStateCreateInfo& rasterizer, VkPipelineMultisampleStateCreateInfo& multisampling, VkPipelineColorBlendStateCreateInfo& colorBlending, VkPipelineDynamicStateCreateInfo& dynamicState);

    GLFWwindow* m_window = nullptr;
    
    std::unique_ptr<VulkanDevice> m_device;
    std::unique_ptr<VulkanSwapChain> m_swapChain;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    std::vector<VkCommandBuffer> m_commandBuffers;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

    uint32_t m_currentFrame = 0;
    bool m_framebufferResized = false;

    const int MAX_FRAMES_IN_FLIGHT = 2;
};

} // namespace vroom
