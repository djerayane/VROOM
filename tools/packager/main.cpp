#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstring>
#include "vroom/asset/PackageFormat.hpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_directory> <output_package>" << std::endl;
        return 1;
    }

    fs::path inputDir = argv[1];
    fs::path outputFile = argv[2];

    if (!fs::exists(inputDir) || !fs::is_directory(inputDir)) {
        std::cerr << "Error: Input directory does not exist or is not a directory." << std::endl;
        return 1;
    }

    std::vector<std::pair<fs::path, std::string>> files;
    for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
        if (entry.is_regular_file()) {
            fs::path relativePath = fs::relative(entry.path(), inputDir);
            if (relativePath.string().length() >= 256) {
                std::cerr << "Warning: Skipping file with path too long: " << relativePath.string() << std::endl;
                continue;
            }
            files.push_back({entry.path(), relativePath.string()});
        }
    }

    std::ofstream out(outputFile, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error: Failed to open output file." << std::endl;
        return 1;
    }

    // Prepare header
    vroom::PackageHeader header;
    header.fileCount = static_cast<uint32_t>(files.size());
    
    // Calculate offsets
    std::vector<vroom::PackageFileEntry> entries;
    uint64_t currentOffset = sizeof(vroom::PackageHeader) + (files.size() * sizeof(vroom::PackageFileEntry));

    for (const auto& [path, relativePath] : files) {
        vroom::PackageFileEntry entry;
        std::memset(entry.path, 0, sizeof(entry.path));
        std::strncpy(entry.path, relativePath.c_str(), sizeof(entry.path) - 1);
        entry.offset = currentOffset;
        entry.size = fs::file_size(path);
        
        entries.push_back(entry);
        currentOffset += entry.size;
    }

    // Write header
    out.write(reinterpret_cast<char*>(&header), sizeof(header));

    // Write file entries
    for (const auto& entry : entries) {
        out.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
    }

    // Write file contents
    for (const auto& [path, relativePath] : files) {
        std::ifstream in(path, std::ios::binary);
        out << in.rdbuf();
    }

    std::cout << "Package created successfully: " << outputFile << " (" << files.size() << " files)" << std::endl;

    return 0;
}

