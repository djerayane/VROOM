#include "vroom/asset/ObjLoader.hpp"
#include "vroom/logging/LogMacros.hpp"
#include <sstream>
#include <stdexcept>

namespace vroom {

static void parsePosition(std::stringstream& lineSS, std::vector<glm::vec3>& positions) {
    glm::vec3 pos;
    lineSS >> pos.x >> pos.y >> pos.z;
    pos.y = -pos.y;
    positions.push_back(pos);
}

static void parseNormal(std::stringstream& lineSS, std::vector<glm::vec3>& normals) {
    glm::vec3 normal;
    lineSS >> normal.x >> normal.y >> normal.z;
    normals.push_back(normal);
}

static void parseTexCoord(std::stringstream& lineSS, std::vector<glm::vec2>& texCoords) {
    glm::vec2 tc;
    lineSS >> tc.x >> tc.y;
    texCoords.push_back(tc);
}

static void triangulateFace(const std::vector<Vertex>& faceVertices, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
    for (size_t i = 1; i < faceVertices.size() - 1; i++) {
        vertices.push_back(faceVertices[0]);
        indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
        vertices.push_back(faceVertices[i]);
        indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
        vertices.push_back(faceVertices[i + 1]);
        indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
    }
}

static void parseFace(std::stringstream& lineSS, const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals, const std::vector<glm::vec2>& texCoords, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, const std::string& path) {
    std::string vertexStr;
    std::vector<Vertex> faceVertices;
    while (lineSS >> vertexStr) {
        int posIdx = -1, tcIdx = -1, normIdx = -1;
        std::replace(vertexStr.begin(), vertexStr.end(), '/', ' ');
        std::stringstream vss(vertexStr);
        vss >> posIdx;
        if (vss.peek() != EOF) vss >> tcIdx;
        if (vss.peek() != EOF) vss >> normIdx;
        if (posIdx > 0 && posIdx <= static_cast<int>(positions.size())) {
            Vertex v{};
            v.position = positions[posIdx - 1];
            if (normIdx > 0 && normIdx <= static_cast<int>(normals.size())) v.normal = normals[normIdx - 1];
            if (tcIdx > 0 && tcIdx <= static_cast<int>(texCoords.size())) v.texCoord = texCoords[tcIdx - 1];
            faceVertices.push_back(v);
        }
    }
    if (faceVertices.size() >= 3) {
        triangulateFace(faceVertices, vertices, indices);
    }
}

std::shared_ptr<Mesh> ObjLoader::load(const std::vector<char>& data, const std::string& path) {
    std::string content(data.begin(), data.end());
    std::stringstream ss(content);
    std::string line;
    std::vector<glm::vec3> positions, normals;
    std::vector<glm::vec2> texCoords;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    while (std::getline(ss, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::stringstream lineSS(line);
        std::string type;
        lineSS >> type;
        if (type == "v") parsePosition(lineSS, positions);
        else if (type == "vn") parseNormal(lineSS, normals);
        else if (type == "vt") parseTexCoord(lineSS, texCoords);
        else if (type == "f") parseFace(lineSS, positions, normals, texCoords, vertices, indices, path);
    }
    LOG_ENGINE_STATIC_INFO("ObjLoader", "Loaded OBJ mesh: " + path + " with " + std::to_string(vertices.size()) + " vertices and " + std::to_string(indices.size()) + " indices.");
    return std::make_shared<Mesh>(std::move(vertices), std::move(indices));
}

} // namespace vroom
