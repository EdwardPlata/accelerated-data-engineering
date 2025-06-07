#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>

// Simplified SFTP client for demonstration (without libssh2 dependency)

namespace etl {

struct SftpFileInfo {
    std::string name;
    std::string path;
    bool is_directory;
    size_t size;
    std::string permissions;
    std::string modified_time;
};

struct SftpTransferResult {
    bool success;
    std::string error_message;
    size_t bytes_transferred;
    double transfer_time;
    std::string local_path;
    std::string remote_path;
};

class SftpClient {
public:
    SftpClient();
    ~SftpClient();

    // Connection management
    bool connect(const std::string& hostname, int port, 
                const std::string& username, const std::string& password);
    bool connectWithKey(const std::string& hostname, int port, 
                       const std::string& username, const std::string& privateKeyPath);
    void disconnect();
    bool isConnected() const;

    // File operations
    SftpTransferResult uploadFile(const std::string& localPath, const std::string& remotePath);
    SftpTransferResult downloadFile(const std::string& remotePath, const std::string& localPath);
    
    // Directory operations
    std::vector<SftpFileInfo> listDirectory(const std::string& remotePath);
    bool createDirectory(const std::string& remotePath);
    bool removeDirectory(const std::string& remotePath);
    bool directoryExists(const std::string& remotePath);
    
    // File management
    bool deleteFile(const std::string& remotePath);
    bool fileExists(const std::string& remotePath);
    SftpFileInfo getFileInfo(const std::string& remotePath);
    bool renameFile(const std::string& oldPath, const std::string& newPath);
    
    // Batch operations
    struct BatchTransferResult {
        int successful_transfers;
        int failed_transfers;
        std::vector<std::string> failed_files;
        double total_time;
        size_t total_bytes;
    };
    
    BatchTransferResult uploadDirectory(const std::string& localDirectory, 
                                       const std::string& remoteDirectory,
                                       bool recursive = true);
    
    BatchTransferResult downloadDirectory(const std::string& remoteDirectory, 
                                         const std::string& localDirectory,
                                         bool recursive = true);
    
    // Configuration
    void setTimeout(int timeoutSeconds);
    void setBufferSize(size_t bufferSize);
    void enableCompression(bool enable);
    
    // Progress callback for transfers
    void setProgressCallback(const std::function<void(size_t transferred, size_t total)>& callback);

private:
    std::string hostname_;
    int port_;
    std::string username_;
    std::string password_;
    std::string private_key_path_;
    bool connected_;
    int timeout_;
    size_t buffer_size_;
    bool compression_enabled_;
    
    std::function<void(size_t, size_t)> progress_callback_;
    
    // Helper methods
    void simulateOperation(const std::string& operation, const std::string& path);
    std::vector<std::string> getFilesInDirectory(const std::string& directory, bool recursive = false);
    bool createLocalDirectory(const std::string& path);
    void simulateTransfer(const std::string& source, const std::string& destination, size_t size);
};

} // namespace etl
