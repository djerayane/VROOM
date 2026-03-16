#pragma once

#include <memory>
#include "vroom/core/SceneManager.hpp"
#include "vroom/asset/AssetManager.hpp"

#ifdef VROOM_WITH_IMGUI
#include <functional>
#endif

// Forward declarations
struct GLFWwindow;

namespace vroom {

class VulkanRenderer;

struct EngineConfig {
    bool headless = false;
    int windowWidth = 800;
    int windowHeight = 600;
    const char* windowTitle = "VROOM Engine";
};

class Engine {
public:
    Engine(const EngineConfig& config = EngineConfig());
    ~Engine();

    /// \brief Updates the engine systems.
    /// \param deltaTime Time elapsed since the last frame.
    void update(float deltaTime);

    /// \brief Runs the main engine loop.
    void run();

    /// \brief Stops the engine loop.
    void stop();

    /// \brief Gets the scene manager.
    /// \return Reference to the scene manager.
    SceneManager& getSceneManager() { return *m_sceneManager; }

    /// \brief Gets the asset manager.
    /// \return Reference to the asset manager.
    AssetManager& getAssetManager() { return *m_assetManager; }

    /// \brief Gets the renderer.
    VulkanRenderer* getRenderer() const { return m_renderer.get(); }

    static Engine& getInstance() { return *s_instance; }
    static Engine* tryGetInstance() { return s_instance; }

#ifdef VROOM_WITH_IMGUI
    /// \brief Registers a callback invoked each frame for ImGui UI rendering.
    void setImGuiCallback(std::function<void()> callback) { m_imguiCallback = std::move(callback); }
#endif

private:
    void initWindow();
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    EngineConfig m_config;
    std::shared_ptr<SceneManager> m_sceneManager;
    std::shared_ptr<AssetManager> m_assetManager;
    bool m_isRunning;

    GLFWwindow* m_window = nullptr;
    std::unique_ptr<VulkanRenderer> m_renderer;

    static Engine* s_instance;

#ifdef VROOM_WITH_IMGUI
    std::function<void()> m_imguiCallback;
#endif
};

} // namespace vroom
