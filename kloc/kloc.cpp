// kloc.cpp
// 31 January 2026 robin.rowe@heroicrobots.com
// License open source MIT

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iomanip>

namespace fs = std::filesystem;

struct FileStats {
    std::string filename;
    int lineCount;
    bool isTooLong;
};

struct ExtensionStats {
    int fileCount = 0;
    int totalLines = 0;
    int tooLongCount = 0;
};

class KlocCounter {
private:
    std::map<std::string, ExtensionStats> extensionStats;
    std::map<std::string, ExtensionStats> directoryStats;
    std::map<std::string, std::vector<FileStats>> directoryFiles;
    
    // Accepted file extensions
    const std::vector<std::string> validExtensions = {".cpp", ".cc", ".cxx", ".c++", ".cp", ".c", ".h", ".hpp", ".hh", ".hxx", ".h++", ".hp"};
    
    bool isValidExtension(const std::string& extension) {
        return std::find(validExtensions.begin(), validExtensions.end(), extension) != validExtensions.end();
    }
    
    bool isEmptyOrWhitespace(const std::string& line) {
        return line.find_first_not_of(" \t\n\r") == std::string::npos;
    }
    
    int countLinesInFile(const fs::path& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Warning: Could not open file " << filepath << std::endl;
            return 0;
        }
        
        int count = 0;
        std::string line;
        
        while (std::getline(file, line)) {
            if (!isEmptyOrWhitespace(line)) {
                count++;
            }
        }
        
        return count;
    }
    
    void processFile(const fs::path& filepath) {
        std::string extension = filepath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        if (!isValidExtension(extension)) {
            return;
        }
        
        int lineCount = countLinesInFile(filepath);
        bool isTooLong = lineCount >= 1000;
        
        std::string dirPath = filepath.parent_path().string();
        if (dirPath.empty()) dirPath = ".";
        
        // Update extension stats
        extensionStats[extension].fileCount++;
        extensionStats[extension].totalLines += lineCount;
        if (isTooLong) extensionStats[extension].tooLongCount++;
        
        // Update directory stats
        directoryStats[dirPath].fileCount++;
        directoryStats[dirPath].totalLines += lineCount;
        if (isTooLong) directoryStats[dirPath].tooLongCount++;
        
        // Store file info for directory display
        directoryFiles[dirPath].push_back({filepath.filename().string(), lineCount, isTooLong});
    }
    
    void processDirectory(const fs::path& directory) {
        try {
            for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    processFile(entry.path());
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error accessing directory " << directory << ": " << e.what() << std::endl;
        }
    }
    
    void printDirectoryReport(const std::string& dirPath) {
        std::cout << "\n=== Directory: " << dirPath << " ===" << std::endl;
        
        // Group files by extension for this directory
        std::map<std::string, std::vector<FileStats>> filesByExtension;
        for (const auto& file : directoryFiles[dirPath]) {
            std::string ext = fs::path(file.filename).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            filesByExtension[ext].push_back(file);
        }
        
        for (const auto& [ext, files] : filesByExtension) {
            std::cout << "\n" << ext << " files:" << std::endl;
            for (const auto& file : files) {
                std::cout << "  " << std::left << std::setw(30) << file.filename 
                          << std::right << std::setw(8) << file.lineCount;
                if (file.isTooLong) {
                    std::cout << "  (too long)";
                }
                std::cout << std::endl;
            }
        }
        
        const auto& stats = directoryStats[dirPath];
        std::cout << "\nDirectory totals: " << stats.fileCount << " files, "
                  << stats.totalLines << " lines, "
                  << stats.tooLongCount << " too long files" << std::endl;
    }
    
    void printFinalReport() {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "FINAL REPORT" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        int totalFiles = 0;
        int totalLines = 0;
        int totalTooLong = 0;
        
        std::cout << "\nBy extension type:" << std::endl;
        std::cout << std::left << std::setw(10) << "Extension" 
                  << std::right << std::setw(10) << "Files" 
                  << std::setw(12) << "Lines" 
                  << std::setw(15) << "Too Long" << std::endl;
        std::cout << std::string(47, '-') << std::endl;
        
        for (const auto& [ext, stats] : extensionStats) {
            std::cout << std::left << std::setw(10) << ext 
                      << std::right << std::setw(10) << stats.fileCount 
                      << std::setw(12) << stats.totalLines 
                      << std::setw(15) << stats.tooLongCount << std::endl;
            
            totalFiles += stats.fileCount;
            totalLines += stats.totalLines;
            totalTooLong += stats.tooLongCount;
        }
        
        std::cout << std::string(47, '=') << std::endl;
        std::cout << std::left << std::setw(10) << "TOTAL" 
                  << std::right << std::setw(10) << totalFiles 
                  << std::setw(12) << totalLines 
                  << std::setw(15) << totalTooLong << std::endl;
        
        std::cout << "\nGrand totals: " << totalFiles << " files, "
                  << totalLines << " lines of code, "
                  << totalTooLong << " too long files" << std::endl;
    }
    
public:
    void run(int argc, char* argv[]) {
        std::vector<std::string> directories;
        
        // Parse command line arguments
        if (argc == 1) {
            // No directory specified, use current directory
            directories.push_back(".");
        } else {
            for (int i = 1; i < argc; i++) {
                directories.push_back(argv[i]);
            }
        }
        
        std::cout << "Kloc - C/C++ Source Code Line Counter" << std::endl;
        std::cout << "Looking for files with extensions: ";
        for (size_t i = 0; i < validExtensions.size(); i++) {
            std::cout << validExtensions[i];
            if (i < validExtensions.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        std::cout << "Files with 1000+ lines are marked as 'too long'" << std::endl;
        
        // Process each directory
        for (const auto& dir : directories) {
            if (!fs::exists(dir)) {
                std::cerr << "Error: Directory '" << dir << "' does not exist." << std::endl;
                continue;
            }
            
            if (!fs::is_directory(dir)) {
                std::cerr << "Error: '" << dir << "' is not a directory." << std::endl;
                continue;
            }
            
            processDirectory(dir);
        }
        
        // Print directory reports
        for (const auto& [dirPath, files] : directoryFiles) {
            if (!files.empty()) {
                printDirectoryReport(dirPath);
            }
        }
        
        // Print final report
        printFinalReport();
    }
};

int main(int argc, char* argv[]) {
    KlocCounter counter;
    counter.run(argc, argv);
    return 0;
}