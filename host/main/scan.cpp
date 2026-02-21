#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
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
    int year;
    std::string extension;
    size_t fileSize;
};

FileMetadata Audio_Func(const fs::path& filePath) {
    FileMetadata meta;
    meta.path = filePath.string();
    meta.extension = filePath.extension().string();
    meta.fileSize = fs::file_size(filePath);
    
    static const std::vector<std::string> audioExts = {
        ".mp3", ".flac", ".ogg", ".wav", ".m4a", ".aac", ".wma"
    };
    
    bool isAudioFile = std::find(audioExts.begin(), audioExts.end(), meta.extension) != audioExts.end();
    
    if (isAudioFile) {
        TagLib::FileRef f(filePath.c_str());
        if (!f.isNull() && f.tag()) {
            TagLib::Tag* tag = f.tag();
            meta.title = tag->title().toCString(true);
            meta.artist = tag->artist().toCString(true);
            meta.album = tag->album().toCString(true);
            meta.year = tag->year();
        }
    }
    
    return meta;
}

FileMetadata Video_Func(const fs::path& filePath) {
    meta.path = filePath.string
}

class FileScanner {
private:
    std::vector<FileMetadata> indexedFiles;
    
public:
    void scanDirectory(const std::string& directoryPath) {
        indexedFiles.clear();
        
        if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
            std::cerr << "Error: Directory does not exist: " << directoryPath << std::endl;
            return;
        }
        
        try {
            for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
                if (entry.is_regular_file()) {
                    FileMetadata meta = Audio_Func(entry.path());
                    indexedFiles.push_back(meta);
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
        }
    }
    
    const std::vector<FileMetadata>& getIndexedFiles() const {
        return indexedFiles;
    }
    
    void printMetadata() const {
        std::cout << "Indexed " << indexedFiles.size() << " files:\n" << std::endl;
        
        for (const auto& file : indexedFiles) {
            std::cout << "File: " << file.path << std::endl;
            std::cout << "  Size: " << file.fileSize << " bytes" << std::endl;
            std::cout << "  Extension: " << file.extension << std::endl;
            
            if (!file.title.empty() || !file.artist.empty() || !file.album.empty()) {
                std::cout << "  Metadata:" << std::endl;
                if (!file.title.empty()) std::cout << "    Title: " << file.title << std::endl;
                if (!file.artist.empty()) std::cout << "    Artist: " << file.artist << std::endl;
                if (!file.album.empty()) std::cout << "    Album: " << file.album << std::endl;
                if (file.year > 0) std::cout << "    Year: " << file.year << std::endl;
            }
            std::cout << std::endl;
        }
    }
    
    std::vector<FileMetadata> searchByArtist(const std::string& artist) const {
        std::vector<FileMetadata> results;
        for (const auto& file : indexedFiles) {
            if (file.artist == artist) {
                results.push_back(file);
            }
        }
        return results;
    }
    
    std::vector<FileMetadata> searchByAlbum(const std::string& album) const {
        std::vector<FileMetadata> results;
        for (const auto& file : indexedFiles) {
            if (file.album == album) {
                results.push_back(file);
            }
        }
        return results;
    }
};

int main() {
    FileScanner scanner;
    
    std::string directoryPath = ".";
    std::cout << "Enter directory path to scan (default: current directory): ";
    std::getline(std::cin, directoryPath);
    
    if (directoryPath.empty()) {
        directoryPath = ".";
    }
    
    std::cout << "Scanning directory: " << directoryPath << std::endl;
    scanner.scanDirectory(directoryPath);
    
    scanner.printMetadata();
    
    return 0;
}