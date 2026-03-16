#include "vroom/core/Engine.hpp"
#include "vroom/logging/LogMacros.hpp"
#include "vroom/core/Version.hpp"
#include "vroom/core/Platform.hpp"
#include "vroom/vulkan/VulkanRenderer.hpp"
#include "vroom/asset/AssetProvider.hpp"
#include "vroom/asset/ShaderAsset.hpp"
#include "vroom/asset/ShaderCompiler.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef VROOM_WITH_IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#endif

#include <chrono>
#include <stdexcept>
#include <thread>
#include <filesystem>

namespace vroom {

Engine* Engine::s_instance = nullptr;

Engine::Engine(const EngineConfig& config)
    : m_config(config), m_isRunning(false) {
    if (s_instance) {
        throw std::runtime_error("Engine instance already exists!");
    }
    s_instance = this;
    LOG_ENGINE_INFO("Initializing VROOM Engine v" + Version::getVersionString() + " (" + Version::GIT_HASH + ")");
    
    // Initialize Asset Manager
    m_assetManager = std::make_unique<AssetManager>();
    
    // Initialize default shader compiler
    m_assetManager->setShaderCompiler(std::make_unique<SystemShaderCompiler>());

    // Register ShaderAsset loader
    m_assetManager->registerLoader<ShaderAsset>([this](const std::vector<char>& data, const std::string& path) -> std::shared_ptr<ShaderAsset> {
        std::string ext = std::filesystem::path(path).extension().string();
        ShaderStage stage = ShaderStage::Unknown;

        if (ext == ".vert" || ext == ".vs") stage = ShaderStage::Vertex;
        else if (ext == ".frag" || ext == ".fs") stage = ShaderStage::Fragment;
        else if (ext == ".comp") stage = ShaderStage::Compute;
        else if (ext == ".geom") stage = ShaderStage::Geometry;
        else if (ext == ".tesc") stage = ShaderStage::TessellationControl;
        else if (ext == ".tese") stage = ShaderStage::TessellationEvaluation;
        
        // Check if it's already a SPIR-V binary
        if (ext == ".spv") {
            // Can't easily determine stage from just binary without parsing SPIR-V or relying on naming convention
            // For now, assume naming convention shader.vert.spv
             std::string stem = std::filesystem::path(path).stem().extension().string();
             if (stem == ".vert") stage = ShaderStage::Vertex;
             else if (stem == ".frag") stage = ShaderStage::Fragment;
             
             return std::make_shared<ShaderAsset>(data, stage);
        }

        // It's source code, compile it
        auto compiler = m_assetManager->getShaderCompiler();
        if (compiler) {
            std::string source(data.begin(), data.end());
            auto spv = compiler->compile(path, source, stage);
            if (spv) {
                return std::make_shared<ShaderAsset>(*spv, stage);
            }
        } else {
            LOG_ENGINE_ERROR("No shader compiler available to compile: " + path);
        }

        return nullptr;
    });

    // Look for assets package relative to executable
    auto baseDir = Platform::getExecutableDir();
    auto assetsPackage = baseDir / "assets.vrpk";
    
    if (std::filesystem::exists(assetsPackage)) {
        LOG_ENGINE_INFO("Found assets package: " + assetsPackage.string());
        m_assetManager->addProvider(std::make_unique<PackageAssetProvider>(assetsPackage));
    } else {
        // Fallback to directory relative to executable or CWD
        auto defaultAssetsPath = baseDir / "assets";
        if (!std::filesystem::exists(defaultAssetsPath)) {
             defaultAssetsPath = std::filesystem::current_path() / "assets";
        }

        if (std::filesystem::exists(defaultAssetsPath)) {
            LOG_ENGINE_INFO("Found default assets directory: " + defaultAssetsPath.string());
            m_assetManager->addProvider(std::make_unique<DiskAssetProvider>(defaultAssetsPath));
        } else {
            LOG_ENGINE_WARNING("Default assets not found (checked assets.vrpk and ./assets)");
        }
    }

    // Look for engine assets package relative to executable
    auto enginePackage = baseDir / "engine/engine.vrpk";
    if (std::filesystem::exists(enginePackage)) {
        LOG_ENGINE_INFO("Found engine package: " + enginePackage.string());
        m_assetManager->addProvider(std::make_unique<PackageAssetProvider>(enginePackage));
    } else {
        // Fallback to directory
        auto engineAssetsPath = baseDir / "engine";
        if (!std::filesystem::exists(engineAssetsPath)) {
            engineAssetsPath = std::filesystem::current_path() / "engine";
        }

        if (std::filesystem::exists(engineAssetsPath)) {
            LOG_ENGINE_INFO("Found engine assets directory: " + engineAssetsPath.string());
            m_assetManager->addProvider(std::make_unique<DiskAssetProvider>(engineAssetsPath));
        }
    }

    if (!m_config.headless) {
        initWindow();
#ifdef VROOM_WITH_IMGUI
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForVulkan(m_window, true);
#endif

        m_renderer = std::make_unique<VulkanRenderer>(*m_assetManager);
        try {
            m_renderer->init(m_window);
        } catch (const std::exception& e) {
            LOG_ENGINE_ERROR("Failed to initialize Vulkan renderer: " + std::string(e.what()));
            throw;
        }
    } else {
        LOG_ENGINE_INFO("Running in HEADLESS mode. Graphics system disabled.");
    }
    m_sceneManager = std::make_shared<SceneManager>();
    m_assetManager = std::make_shared<AssetManager>();
}

Engine::~Engine() {
    if (m_renderer) {
        m_renderer->deviceWaitIdle();
    }
    m_sceneManager.reset();
    m_assetManager.reset();
    if (m_renderer) {
        m_renderer.reset();
    }
#ifdef VROOM_WITH_IMGUI
    if (!m_config.headless) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
#endif
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    if (!m_config.headless) {
        glfwTerminate();
    }

    m_sceneManager.reset();
    m_assetManager.reset();
    LOG_ENGINE_INFO("Engine shutdown complete, goodbye!");
}

void Engine::initWindow() {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    m_window = glfwCreateWindow(m_config.windowWidth, m_config.windowHeight, m_config.windowTitle, nullptr, nullptr);
    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

void Engine::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto engine = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (engine && engine->m_renderer) {
        engine->m_renderer->setFramebufferResized(true);
    }
}

void Engine::update(float deltaTime) {
    if (m_sceneManager) {
        m_sceneManager->update(deltaTime);
    }
}

void Engine::run() {
    m_isRunning = true;
    auto lastTime = std::chrono::high_resolution_clock::now();
    LOG_ENGINE_INFO("Starting engine loop...");
    while (m_isRunning) {
        if (m_window && !glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
        } else if (m_window && glfwWindowShouldClose(m_window)) {
            m_isRunning = false;
        }
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        update(deltaTime);
        if (m_renderer && m_window) {
            try {
#ifdef VROOM_WITH_IMGUI
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();
                if (m_imguiCallback) {
                    m_imguiCallback();
                }
                ImGui::Render();
#endif
                m_renderer->drawFrame(m_sceneManager->getActiveScene());
            } catch (const std::exception& e) {
                LOG_ENGINE_ERROR("Render error: " + std::string(e.what()));
                m_isRunning = false;
            }
        } else {
            // In headless mode, we need to sleep a bit to avoid burning 100% CPU
            // as there is no vsync or swapchain waiting.
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
    }
    if (m_renderer) {
        m_renderer->deviceWaitIdle();
    }
    LOG_ENGINE_INFO("Engine loop stopped.");
}

void Engine::stop() {
    m_isRunning = false;
}

} // namespace vroom
