#pragma once

#include "vroom/asset/Asset.hpp"
#include <vector>
#include <string>

namespace vroom {

/**
 * @brief Enum representing the shader stage.
 */
enum class ShaderStage {
    Vertex,
    Fragment,
    Compute,
    Geometry,
    TessellationControl,
    TessellationEvaluation,
    Unknown
};

/**
 * @brief Asset representing a compiled shader.
 * Holds the SPIR-V binary data.
 */
class ShaderAsset : public Asset {
public:
    ShaderAsset(std::vector<char> data, ShaderStage stage)
        : m_data(std::move(data)), m_stage(stage) {}

    [[nodiscard]] const std::vector<char>& getData() const { return m_data; }
    [[nodiscard]] ShaderStage getStage() const { return m_stage; }

private:
    std::vector<char> m_data;
    ShaderStage m_stage;
};

} // namespace vroom
