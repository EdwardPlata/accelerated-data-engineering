#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <libssh2.h>
#include <libssh2_sftp.h>

namespace etl {

struct SftpFileInfo {
    std::string name;
    std::string path;
    bool is_directory;
    long long size;
    unsigned long permissions;
    time_t modified_time;
    time_t access_time;
};

struct SftpTransferResult {
    bool success;
    std::string error_message;
    size_t bytes_transferred;
    double transfer_time;
    std::string local_path;
    std::string remote_path;
};

struct SftpConnectionInfo {
    std::string hostname;
    int port;
    std::string username;
    std::string password;
    std::string private_key_path;
    std::string public_key_path;
    std::string passphrase;
    int timeout_seconds;
};

class SftpClient {
public:
    SftpClient();
    ~SftpClient();

    // Connection management
    bool connect(const SftpConnectionInfo& connInfo);
    bool connectWithPassword(const std::string& hostname, int port, 
                           const std::string& username, const std::string& password);
    bool connectWithKeyFile(const std::string& hostname, int port, 
                          const std::string& username, const std::string& privateKeyPath,
                          const std::string& publicKeyPath = "", const std::string& passphrase = "");
    void disconnect();
    bool isConnected() const;
    
    // File operations
    SftpTransferResult uploadFile(const std::string& localFilePath, const std::string& remoteFilePath);
    SftpTransferResult downloadFile(const std::string& remoteFilePath, const std::string& localFilePath);
    SftpTransferResult uploadData(const std::string& data, const std::string& remoteFilePath);
    std::string downloadToMemory(const std::string& remoteFilePath);
    
    // File operations with progress
    SftpTransferResult uploadFileWithProgress(const std::string& localFilePath, 
                                             const std::string& remoteFilePath,
                                             const std::function<void(size_t, size_t)>& progressCallback);
    SftpTransferResult downloadFileWithProgress(const std::string& remoteFilePath, 
                                               const std::string& localFilePath,
                                               const std::function<void(size_t, size_t)>& progressCallback);
    
    // Directory operations
    std::vector<SftpFileInfo> listDirectory(const std::string& remotePath);
    std::vector<std::string> listFiles(const std::string& remotePath, bool recursive = false);
    bool createDirectory(const std::string& remotePath);
    bool removeDirectory(const std::string& remotePath);
    bool changeDirectory(const std::string& remotePath);
    std::string getCurrentDirectory();
    
    // File management
    bool deleteFile(const std::string& remoteFilePath);
    bool fileExists(const std::string& remoteFilePath);
    SftpFileInfo getFileInfo(const std::string& remoteFilePath);
    bool renameFile(const std::string& oldPath, const std::string& newPath);
    
    // Batch operations for ETL
    struct BatchTransferResult {
        int successful_transfers;
        int failed_transfers;
        std::vector<std::string> failed_files;
        double total_time;
        size_t total_bytes;
    };
    
    BatchTransferResult uploadDirectory(const std::string& localDirectory, 
                                       const std::string& remoteDirectory,
                                       const std::string& filePattern = "*",
                                       bool recursive = true);
    
    BatchTransferResult downloadDirectory(const std::string& remoteDirectory, 
                                         const std::string& localDirectory,
                                         const std::string& filePattern = "*",
                                         bool recursive = true);
    
    BatchTransferResult uploadFiles(const std::vector<std::string>& localFiles, 
                                   const std::string& remoteDirectory);
    
    BatchTransferResult downloadFiles(const std::vector<std::string>& remoteFiles, 
                                     const std::string& localDirectory);
    
    // Utility methods
    void setTimeout(int timeoutSeconds);
    void setTransferMode(bool binaryMode = true);
    std::string getLastError() const;
    
    // ETL-specific methods
    std::vector<std::string> findFiles(const std::string& remotePath, 
                                      const std::string& pattern, 
                                      bool recursive = false);
    std::vector<SftpFileInfo> findFilesByDate(const std::string& remotePath, 
                                             time_t startDate, time_t endDate,
                                             bool recursive = false);
    
private:
    LIBSSH2_SESSION* session_;
    LIBSSH2_SFTP* sftp_session_;
    int socket_;
    bool connected_;
    std::string last_error_;
    int timeout_seconds_;
    bool binary_mode_;
    
    // Helper methods
    bool initializeLibssh2();
    void cleanupLibssh2();
    bool establishSocket(const std::string& hostname, int port);
    void closeSocket();
    std::string getLibssh2Error();
    size_t getFileSize(const std::string& filePath);
    bool createDirectoryRecursive(const std::string& remotePath);
    std::vector<std::string> getFilesInLocalDirectory(const std::string& directory, 
                                                     const std::string& pattern = "*",
                                                     bool recursive = true);
    bool matchesPattern(const std::string& filename, const std::string& pattern);
};

} // namespace etl
