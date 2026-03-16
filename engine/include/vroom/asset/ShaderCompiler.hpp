#pragma once

#include "vroom/asset/ShaderAsset.hpp"
#include <filesystem>
#include <optional>
#include <vector>

namespace vroom {

/**
 * @brief Interface for compiling shaders at runtime.
 */
class ShaderCompiler {
public:
    virtual ~ShaderCompiler() = default;

    /**
     * @brief Compiles a GLSL shader source to SPIR-V.
     * @param sourcePath Path to the source file (used for debugging and include resolution).
     * @param sourceCode The GLSL source code.
     * @param stage The shader stage.
     * @return The compiled SPIR-V binary, or nullopt if compilation failed.
     */
    [[nodiscard]] virtual std::optional<std::vector<char>> compile(
        const std::filesystem::path& sourcePath,
        const std::string& sourceCode,
        ShaderStage stage
    ) = 0;
};

/**
 * @brief Implementation of ShaderCompiler that invokes the system 'glslc' command.
 * This assumes Vulkan SDK is installed and glslc is in the PATH.
 */
class SystemShaderCompiler : public ShaderCompiler {
public:
    SystemShaderCompiler();

    [[nodiscard]] std::optional<std::vector<char>> compile(
        const std::filesystem::path& sourcePath,
        const std::string& sourceCode,
        ShaderStage stage
    ) override;
};

} // namespace vroom
