#pragma once

#include "vroom/asset/Mesh.hpp"
#include <memory>
#include <vector>
#include <string>

namespace vroom {

class ObjLoader {
public:
    /**
     * @brief Loads a mesh from OBJ data.
     * @param data The raw file data.
     * @param path Optional path for debugging.
     * @return Shared pointer to the loaded Mesh.
     */
    static std::shared_ptr<Mesh> load(const std::vector<char>& data, const std::string& path = "");
};

} // namespace vroom

