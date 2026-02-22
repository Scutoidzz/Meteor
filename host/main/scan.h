#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <fstream>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>
#include <algorithm>

namespace fs = std::filesystem;

struct FileMetadata {
    std::string path;
    std::string title;
    std::string artist;
    std::string album;
    std::string duration;
    int year;
    std::string extension;
    size_t fileSize;
};

FileMetadata Audio_Func(const fs::path& filePath);
FileMetadata Video_Func(const fs::path& filePath);

class FileScanner {
private:
    std::vector<FileMetadata> indexedFiles;
    
public:
    void scanDirectory(const std::string& directoryPath);
    void writeMetadataToFile(const std::string& filename) const;
    const std::vector<FileMetadata>& getIndexedFiles() const;
    void printMetadata() const;
    std::vector<FileMetadata> searchByArtist(const std::string& artist) const;
    std::vector<FileMetadata> searchByAlbum(const std::string& album) const;
};
