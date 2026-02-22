#include "scan.h"

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
        meta.duration = ""; // Audio files could have duration extracted with additional libraries
    }
    
    return meta;
}

FileMetadata Video_Func(const fs::path& filePath) {
    FileMetadata meta;
    meta.path = filePath.string();
    meta.extension = filePath.extension().string();
    meta.fileSize = fs::file_size(filePath);
    
    static const std::vector<std::string> videoExts = {
        ".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm", 
        ".m4v", ".mpg", ".mpeg", ".3gp", ".ogv", ".ts", ".m2ts"
    };
    
    bool isVideoFile = std::find(videoExts.begin(), videoExts.end(), meta.extension) != videoExts.end();
    
    if (isVideoFile) {
        // TODO: Finish this up
        meta.title = filePath.stem().string();
        meta.artist = "";
        meta.album = "";
        meta.year = 0;
        meta.duration = "3:32"; //<- I have no idea how to do this so I'm using examples
     }
    
    return meta;
}

// Standalone main function for scan target
#ifndef METEOR_MAIN_APP
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
    
    // Write metadata to file
    std::string outputFilename = "file_metadata_report.txt";
    scanner.writeMetadataToFile(outputFilename);
    
    // Also display to console
    scanner.printMetadata();
    
    return 0;
}
#endif

void FileScanner::scanDirectory(const std::string& directoryPath) {
    indexedFiles.clear();
    
    if (!fs::exists(directoryPath) || !fs::is_directory(directoryPath)) {
        std::cerr << "Error: Directory does not exist: " << directoryPath << std::endl;
        return;
    }
    
    try {
        for (const auto& entry : fs::recursive_directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                FileMetadata meta;
                std::string extension = entry.path().extension().string();
                
                // Transform extension to lowercase for comparison
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                
                // Check if it's an audio file
                static const std::vector<std::string> audioExts = {
                    ".mp3", ".flac", ".ogg", ".wav", ".m4a", ".aac", ".wma"
                };
                static const std::vector<std::string> videoExts = {
                    ".mp4", ".avi", ".mkv", ".mov", ".wmv", ".flv", ".webm", 
                    ".m4v", ".mpg", ".mpeg", ".3gp", ".ogv", ".ts", ".m2ts"
                };
                
                bool isAudioFile = std::find(audioExts.begin(), audioExts.end(), extension) != audioExts.end();
                bool isVideoFile = std::find(videoExts.begin(), videoExts.end(), extension) != videoExts.end();
                
                if (isAudioFile) {
                    meta = Audio_Func(entry.path());
                } else if (isVideoFile) {
                    meta = Video_Func(entry.path());
                } else {
                    // For other files, create basic metadata
                    meta.path = entry.path().string();
                    meta.extension = entry.path().extension().string();
                    meta.fileSize = fs::file_size(entry.path());
                    meta.title = entry.path().stem().string();
                    meta.artist = "";
                    meta.album = "";
                    meta.year = 0;
                    meta.duration = "";
                }
                
                indexedFiles.push_back(meta);
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    }
}
    
void FileScanner::writeMetadataToFile(const std::string& filename) const {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for writing." << std::endl;
        return;
    }
    
    outFile << "File Metadata Report\n";
    outFile << "===================\n\n";
    outFile << "Total files indexed: " << indexedFiles.size() << "\n\n";
    
    for (const auto& file : indexedFiles) {
        outFile << "File: " << file.path << "\n";
        outFile << "  Size: " << file.fileSize << " bytes\n";
        outFile << "  Extension: " << file.extension << "\n";
        outFile << "  Title: " << file.title << "\n";
        
        if (!file.artist.empty()) outFile << "  Artist: " << file.artist << "\n";
        if (!file.album.empty()) outFile << "  Album: " << file.album << "\n";
        if (file.year > 0) outFile << "  Year: " << file.year << "\n";
        
        outFile << "\n";
    }
    
    outFile.close();
    std::cout << "Metadata written to file: " << filename << std::endl;
}
    
const std::vector<FileMetadata>& FileScanner::getIndexedFiles() const {
    return indexedFiles;
}
    
void FileScanner::printMetadata() const {
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
    
std::vector<FileMetadata> FileScanner::searchByArtist(const std::string& artist) const {
    std::vector<FileMetadata> results;
    for (const auto& file : indexedFiles) {
        if (file.artist == artist) {
            results.push_back(file);
        }
    }
    return results;
}
    
std::vector<FileMetadata> FileScanner::searchByAlbum(const std::string& album) const {
    std::vector<FileMetadata> results;
    for (const auto& file : indexedFiles) {
        if (file.album == album) {
            results.push_back(file);
        }
    }
    return results;
}