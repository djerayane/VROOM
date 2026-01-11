#include "vroom/asset/AssetManager.hpp"
#include "vroom/asset/ShaderCompiler.hpp"
#include "vroom/asset/Mesh.hpp"
#include "vroom/asset/ObjLoader.hpp"

namespace vroom {

AssetManager::AssetManager() {
    registerLoader<Mesh>(ObjLoader::load);
}

AssetManager::~AssetManager() = default;

void AssetManager::addProvider(std::unique_ptr<AssetProvider> provider) {
    if (provider) {
        m_providers.push_back(std::move(provider));
    }
}

void AssetManager::clearProviders() {
    m_providers.clear();
    // Also clear the asset cache as references might be invalid or we want to force reload from new providers
    std::lock_guard<std::mutex> lock(m_assetsMutex);
    m_assets.clear();
}

void AssetManager::setShaderCompiler(std::unique_ptr<ShaderCompiler> compiler) {
    m_compiler = std::move(compiler);
}

std::optional<std::vector<char>> AssetManager::readRawAsset(const std::filesystem::path& path) const {
    for (const auto& provider : m_providers) {
        if (provider->exists(path)) {
            return provider->readFile(path);
        }
    }
    return std::nullopt;
}

} // namespace vroom
