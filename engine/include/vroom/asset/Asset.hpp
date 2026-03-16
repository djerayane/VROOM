#pragma once

#include <filesystem>
#include <string>

namespace vroom {

/**
 * @brief Base class for all assets managed by the AssetManager.
 */
class Asset {
public:
    virtual ~Asset() = default;

    [[nodiscard]] const std::filesystem::path& getPath() const { return m_path; }
    void setPath(const std::filesystem::path& path) { m_path = path; }

protected:
    std::filesystem::path m_path;
};

} // namespace vroom
