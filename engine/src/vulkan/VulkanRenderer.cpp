#include "vroom/vulkan/VulkanRenderer.hpp"
#include "vroom/logging/LogMacros.hpp"
#include "vroom/asset/ShaderAsset.hpp"
#include "vroom/core/Scene.hpp"
#include "vroom/asset/Mesh.hpp"
#include "vroom/vulkan/VulkanMeshData.hpp"
#include "vroom/vulkan/VulkanBuffer.hpp"

#ifdef VROOM_WITH_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#endif

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <set>
#include <array>

namespace vroom {

VulkanRenderer::VulkanRenderer(AssetManager& assetManager) 
    : m_assetManager(assetManager) {
}

VulkanRenderer::~VulkanRenderer() {
    deviceWaitIdle();
#ifdef VROOM_WITH_IMGUI
    shutdownImGui();
#endif
    if (m_device) {
        for (auto semaphore : m_renderFinishedSemaphores) {
            vkDestroySemaphore(m_device->getDevice(), semaphore, nullptr);
        }
        m_renderFinishedSemaphores.clear();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (m_imageAvailableSemaphores.size() > i)
                vkDestroySemaphore(m_device->getDevice(), m_imageAvailableSemaphores[i], nullptr);
            if (m_inFlightFences.size() > i)
                vkDestroyFence(m_device->getDevice(), m_inFlightFences[i], nullptr);
        }

        // Order matters: Framebuffers -> DepthResources -> Pipeline -> RenderPass -> SwapChain -> Device
        destroyDepthResources();

        for (auto framebuffer : m_swapChainFramebuffers) {
            vkDestroyFramebuffer(m_device->getDevice(), framebuffer, nullptr);
        }

        vkDestroyPipeline(m_device->getDevice(), m_graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(m_device->getDevice(), m_pipelineLayout, nullptr);
        vkDestroyRenderPass(m_device->getDevice(), m_renderPass, nullptr);
    }

    m_swapChain.reset();
    m_device.reset();
}

void VulkanRenderer::init(GLFWwindow* window) {
    m_window = window;
    m_device = std::make_unique<VulkanDevice>(window);
    m_swapChain = std::make_unique<VulkanSwapChain>(*m_device, window);
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createCommandBuffers();
    createSyncObjects();
#ifdef VROOM_WITH_IMGUI
    initImGui(window);
#endif
}

void VulkanRenderer::recreateSwapChain() {
    m_swapChain->recreate();

    for (auto framebuffer : m_swapChainFramebuffers) {
        vkDestroyFramebuffer(m_device->getDevice(), framebuffer, nullptr);
    }

    destroyDepthResources();
    createDepthResources();
    createFramebuffers();

    // Recreate render finished semaphores as image count might have changed
    for (auto semaphore : m_renderFinishedSemaphores) {
        vkDestroySemaphore(m_device->getDevice(), semaphore, nullptr);
    }
    m_renderFinishedSemaphores.clear();
    m_renderFinishedSemaphores.resize(m_swapChain->getImageCount());

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (size_t i = 0; i < m_renderFinishedSemaphores.size(); i++) {
        if (vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to recreate render finished semaphores!");
        }
    }
}

void VulkanRenderer::deviceWaitIdle() {
    if (m_device) {
        m_device->waitIdle();
    }
}

void VulkanRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_swapChain->getSwapChainImageFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device->getDevice(), &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void VulkanRenderer::createGraphicsPipeline() {
    // Use AssetManager to load shaders
    auto vertShader = m_assetManager.getAsset<ShaderAsset>("shaders/shader.vert");
    auto fragShader = m_assetManager.getAsset<ShaderAsset>("shaders/shader.frag");

    if (!vertShader) {
        throw std::runtime_error("failed to load vertex shader: shaders/shader.vert");
    }
    if (!fragShader) {
        throw std::runtime_error("failed to load fragment shader: shaders/shader.frag");
    }

    VkShaderModule vertShaderModule = createShaderModule(vertShader->getData());
    VkShaderModule fragShaderModule = createShaderModule(fragShader->getData());

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

VkPipelineVertexInputStateCreateInfo VulkanRenderer::createVertexInputState(VkVertexInputBindingDescription& bindingDesc, std::array<VkVertexInputAttributeDescription, 3>& attrDescs) {
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    attrDescs[0].binding = 0;
    attrDescs[0].location = 0;
    attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescs[0].offset = offsetof(Vertex, position);
    attrDescs[1].binding = 0;
    attrDescs[1].location = 1;
    attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescs[1].offset = offsetof(Vertex, normal);
    attrDescs[2].binding = 0;
    attrDescs[2].location = 2;
    attrDescs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrDescs[2].offset = offsetof(Vertex, texCoord);
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs.data();
    return vertexInputInfo;
}

VkPipelineLayoutCreateInfo VulkanRenderer::createPipelineLayoutInfo(VkPushConstantRange& pushConstantRange) {
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    return pipelineLayoutInfo;
}

VkGraphicsPipelineCreateInfo VulkanRenderer::createGraphicsPipelineInfo(VkPipelineShaderStageCreateInfo* shaderStages, VkPipelineVertexInputStateCreateInfo& vertexInput, VkPipelineInputAssemblyStateCreateInfo& inputAssembly, VkPipelineViewportStateCreateInfo& viewportState, VkPipelineRasterizationStateCreateInfo& rasterizer, VkPipelineMultisampleStateCreateInfo& multisampling, VkPipelineColorBlendStateCreateInfo& colorBlending, VkPipelineDynamicStateCreateInfo& dynamicState) {
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    return pipelineInfo;
}

void VulkanRenderer::createGraphicsPipeline() {
    auto vertShaderCode = readFile("engine/shaders/shader.vert.spv");
    auto fragShaderCode = readFile("engine/shaders/shader.frag.spv");
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
    VkPipelineShaderStageCreateInfo shaderStages[2] = {{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main"}, {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main"}};
    VkVertexInputBindingDescription bindingDescription{};
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = createVertexInputState(bindingDescription, attributeDescriptions);
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE};
    VkPipelineViewportStateCreateInfo viewportState{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, nullptr, 0, 1, nullptr, 1, nullptr};
    VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO, nullptr, 0, VK_FALSE, VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0.0f, 0.0f, 0.0f, 1.0f};
    VkPipelineMultisampleStateCreateInfo multisampling{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO, nullptr, 0, VK_SAMPLE_COUNT_1_BIT, VK_FALSE};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{VK_FALSE, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};
    VkPipelineColorBlendStateCreateInfo colorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr, 0, VK_FALSE, VK_LOGIC_OP_COPY, 1, &colorBlendAttachment, {0.0f, 0.0f, 0.0f, 0.0f}};
    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, nullptr, 0, static_cast<uint32_t>(dynamicStates.size()), dynamicStates.data()};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    VkPushConstantRange pushConstantRange{};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = createPipelineLayoutInfo(pushConstantRange);
    if (vkCreatePipelineLayout(m_device->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) throw std::runtime_error("failed to create pipeline layout!");
    VkGraphicsPipelineCreateInfo pipelineInfo = createGraphicsPipelineInfo(shaderStages, vertexInputInfo, inputAssembly, viewportState, rasterizer, multisampling, colorBlending, dynamicState);
    pipelineInfo.pDepthStencilState = &depthStencil;
    if (vkCreateGraphicsPipelines(m_device->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS) throw std::runtime_error("failed to create graphics pipeline!");
    vkDestroyShaderModule(m_device->getDevice(), fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device->getDevice(), vertShaderModule, nullptr);
}

void VulkanRenderer::createFramebuffers() {
    const auto& imageViews = m_swapChain->getImageViews();
    VkExtent2D extent = m_swapChain->getSwapChainExtent();

    m_swapChainFramebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {imageViews[i], m_depthImageView};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device->getDevice(), &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

VkFormat VulkanRenderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_device->getPhysicalDevice(), format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) return format;
        if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) return format;
    }
    throw std::runtime_error("failed to find supported format!");
}

VkFormat VulkanRenderer::findDepthFormat() const {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

void VulkanRenderer::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    VkExtent2D extent = m_swapChain->getSwapChainExtent();
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {extent.width, extent.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateImage(m_device->getDevice(), &imageInfo, nullptr, &m_depthImage) != VK_SUCCESS)
        throw std::runtime_error("failed to create depth image!");
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_device->getDevice(), m_depthImage, &memRequirements);
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_device->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(m_device->getDevice(), &allocInfo, nullptr, &m_depthImageMemory) != VK_SUCCESS)
        throw std::runtime_error("failed to allocate depth image memory!");
    vkBindImageMemory(m_device->getDevice(), m_depthImage, m_depthImageMemory, 0);
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(m_device->getDevice(), &viewInfo, nullptr, &m_depthImageView) != VK_SUCCESS)
        throw std::runtime_error("failed to create depth image view!");
}

void VulkanRenderer::destroyDepthResources() {
    if (m_depthImageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device->getDevice(), m_depthImageView, nullptr);
        m_depthImageView = VK_NULL_HANDLE;
    }
    if (m_depthImage != VK_NULL_HANDLE) {
        vkDestroyImage(m_device->getDevice(), m_depthImage, nullptr);
        m_depthImage = VK_NULL_HANDLE;
    }
    if (m_depthImageMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device->getDevice(), m_depthImageMemory, nullptr);
        m_depthImageMemory = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::createCommandBuffers() {
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_device->getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) m_commandBuffers.size();

    if (vkAllocateCommandBuffers(m_device->getDevice(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, std::shared_ptr<Scene> scene) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapChain->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color.float32[0] = 0.0f;
    clearValues[0].color.float32[1] = 0.0f;
    clearValues[0].color.float32[2] = 0.0f;
    clearValues[0].color.float32[3] = 1.0f;
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) m_swapChain->getSwapChainExtent().width;
        viewport.height = (float) m_swapChain->getSwapChainExtent().height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_swapChain->getSwapChainExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        if (scene) {
            scene->draw(commandBuffer);
        } else {
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        }
#ifdef VROOM_WITH_IMGUI
        if (ImGui::GetDrawData()) {
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        }
#endif
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void VulkanRenderer::createSyncObjects() {
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(m_swapChain->getImageCount());
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(m_device->getDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    for (size_t i = 0; i < m_renderFinishedSemaphores.size(); i++) {
        if (vkCreateSemaphore(m_device->getDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render finished semaphore!");
        }
    }
}

void VulkanRenderer::drawFrame(std::shared_ptr<Scene> scene) {
    vkWaitForFences(m_device->getDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = m_swapChain->acquireNextImage(m_imageAvailableSemaphores[m_currentFrame], &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    vkResetFences(m_device->getDevice(), 1, &m_inFlightFences[m_currentFrame]);

    vkResetCommandBuffer(m_commandBuffers[m_currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
    recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex, scene);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[imageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {m_swapChain->getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(m_device->getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
        m_framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::uploadMesh(std::shared_ptr<Mesh> mesh) {
    if (!mesh || mesh->getGPUData()) return;

    auto meshData = std::make_shared<VulkanMeshData>();
    
    const auto& vertices = mesh->getVertices();
    const auto& indices = mesh->getIndices();

    if (!vertices.empty()) {
        meshData->vertexBuffer = VulkanBuffer::createVertexBuffer(*m_device, vertices.data(), sizeof(Vertex) * vertices.size());
    }

    if (!indices.empty()) {
        meshData->indexBuffer = VulkanBuffer::createIndexBuffer(*m_device, indices.data(), sizeof(uint32_t) * indices.size());
    }

    mesh->setGPUData(meshData);
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device->getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

#ifdef VROOM_WITH_IMGUI
void VulkanRenderer::initImGui(GLFWwindow* window) {
    VkDescriptorPoolSize poolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    if (vkCreateDescriptorPool(m_device->getDevice(), &poolInfo, nullptr, &m_imguiDescriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create ImGui descriptor pool!");
    }
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.ApiVersion = VK_API_VERSION_1_0;
    initInfo.Instance = m_device->getInstance();
    initInfo.PhysicalDevice = m_device->getPhysicalDevice();
    initInfo.Device = m_device->getDevice();
    initInfo.QueueFamily = m_device->findQueueFamilies(m_device->getPhysicalDevice()).graphicsFamily.value();
    initInfo.Queue = m_device->getGraphicsQueue();
    initInfo.DescriptorPool = m_imguiDescriptorPool;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = static_cast<uint32_t>(m_swapChain->getImageCount());
    initInfo.PipelineInfoMain.RenderPass = m_renderPass;
    ImGui_ImplVulkan_Init(&initInfo);
}

void VulkanRenderer::shutdownImGui() {
    if (m_imguiDescriptorPool != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_Shutdown();
        vkDestroyDescriptorPool(m_device->getDevice(), m_imguiDescriptorPool, nullptr);
        m_imguiDescriptorPool = VK_NULL_HANDLE;
    }
}
#endif

} // namespace vroom
