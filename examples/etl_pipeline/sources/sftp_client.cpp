#include "sftp_client.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

namespace etl {

SftpClient::SftpClient() 
    : session_(nullptr), sftp_session_(nullptr), socket_(-1), 
      connected_(false), timeout_seconds_(30), binary_mode_(true) {
    initializeLibssh2();
}

SftpClient::~SftpClient() {
    disconnect();
    cleanupLibssh2();
}

bool SftpClient::initializeLibssh2() {
    int rc = libssh2_init(0);
    if (rc != 0) {
        last_error_ = "Failed to initialize libssh2";
        return false;
    }
    return true;
}

void SftpClient::cleanupLibssh2() {
    libssh2_exit();
}

bool SftpClient::connect(const SftpConnectionInfo& connInfo) {
    if (!connectWithPassword(connInfo.hostname, connInfo.port, 
                            connInfo.username, connInfo.password)) {
        // Try key-based authentication if password fails
        if (!connInfo.private_key_path.empty()) {
            return connectWithKeyFile(connInfo.hostname, connInfo.port, 
                                    connInfo.username, connInfo.private_key_path,
                                    connInfo.public_key_path, connInfo.passphrase);
        }
        return false;
    }
    return true;
}

bool SftpClient::connectWithPassword(const std::string& hostname, int port, 
                                   const std::string& username, const std::string& password) {
    if (connected_) {
        disconnect();
    }
    
    // Establish socket connection
    if (!establishSocket(hostname, port)) {
        return false;
    }
    
    // Create SSH session
    session_ = libssh2_session_init();
    if (!session_) {
        last_error_ = "Failed to create SSH session";
        closeSocket();
        return false;
    }
    
    // Set session timeout
    libssh2_session_set_timeout(session_, timeout_seconds_ * 1000);
    
    // Perform SSH handshake
    int rc = libssh2_session_handshake(session_, socket_);
    if (rc) {
        last_error_ = "SSH handshake failed: " + getLibssh2Error();
        disconnect();
        return false;
    }
    
    // Authenticate with password
    rc = libssh2_userauth_password(session_, username.c_str(), password.c_str());
    if (rc) {
        last_error_ = "Password authentication failed: " + getLibssh2Error();
        disconnect();
        return false;
    }
    
    // Initialize SFTP session
    sftp_session_ = libssh2_sftp_init(session_);
    if (!sftp_session_) {
        last_error_ = "Failed to initialize SFTP session: " + getLibssh2Error();
        disconnect();
        return false;
    }
    
    connected_ = true;
    return true;
}

bool SftpClient::connectWithKeyFile(const std::string& hostname, int port, 
                                  const std::string& username, const std::string& privateKeyPath,
                                  const std::string& publicKeyPath, const std::string& passphrase) {
    if (connected_) {
        disconnect();
    }
    
    // Establish socket connection
    if (!establishSocket(hostname, port)) {
        return false;
    }
    
    // Create SSH session
    session_ = libssh2_session_init();
    if (!session_) {
        last_error_ = "Failed to create SSH session";
        closeSocket();
        return false;
    }
    
    // Set session timeout
    libssh2_session_set_timeout(session_, timeout_seconds_ * 1000);
    
    // Perform SSH handshake
    int rc = libssh2_session_handshake(session_, socket_);
    if (rc) {
        last_error_ = "SSH handshake failed: " + getLibssh2Error();
        disconnect();
        return false;
    }
    
    // Authenticate with key file
    const char* pubkey = publicKeyPath.empty() ? nullptr : publicKeyPath.c_str();
    const char* phrase = passphrase.empty() ? nullptr : passphrase.c_str();
    
    rc = libssh2_userauth_publickey_fromfile(session_, username.c_str(),
                                           pubkey, privateKeyPath.c_str(), phrase);
    if (rc) {
        last_error_ = "Public key authentication failed: " + getLibssh2Error();
        disconnect();
        return false;
    }
    
    // Initialize SFTP session
    sftp_session_ = libssh2_sftp_init(session_);
    if (!sftp_session_) {
        last_error_ = "Failed to initialize SFTP session: " + getLibssh2Error();
        disconnect();
        return false;
    }
    
    connected_ = true;
    return true;
}

void SftpClient::disconnect() {
    if (sftp_session_) {
        libssh2_sftp_shutdown(sftp_session_);
        sftp_session_ = nullptr;
    }
    
    if (session_) {
        libssh2_session_disconnect(session_, "Normal shutdown");
        libssh2_session_free(session_);
        session_ = nullptr;
    }
    
    closeSocket();
    connected_ = false;
}

bool SftpClient::isConnected() const {
    return connected_;
}

bool SftpClient::establishSocket(const std::string& hostname, int port) {
    struct hostent* host = gethostbyname(hostname.c_str());
    if (!host) {
        last_error_ = "Failed to resolve hostname: " + hostname;
        return false;
    }
    
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == -1) {
        last_error_ = "Failed to create socket";
        return false;
    }
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr = *((struct in_addr*)host->h_addr);
    
    if (connect(socket_, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        last_error_ = "Failed to connect to " + hostname + ":" + std::to_string(port);
        closeSocket();
        return false;
    }
    
    return true;
}

void SftpClient::closeSocket() {
    if (socket_ != -1) {
        close(socket_);
        socket_ = -1;
    }
}

std::string SftpClient::getLibssh2Error() {
    char* errmsg;
    int errlen;
    libssh2_session_last_error(session_, &errmsg, &errlen, 0);
    return std::string(errmsg, errlen);
}

SftpTransferResult SftpClient::uploadFile(const std::string& localFilePath, const std::string& remoteFilePath) {
    SftpTransferResult result;
    result.success = false;
    result.local_path = localFilePath;
    result.remote_path = remoteFilePath;
    
    if (!connected_) {
        result.error_message = "Not connected to SFTP server";
        return result;
    }
    
    std::ifstream localFile(localFilePath, std::ios::binary);
    if (!localFile.is_open()) {
        result.error_message = "Cannot open local file: " + localFilePath;
        return result;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Get file size
    localFile.seekg(0, std::ios::end);
    size_t fileSize = localFile.tellg();
    localFile.seekg(0, std::ios::beg);
    
    // Open remote file for writing
    LIBSSH2_SFTP_HANDLE* remoteFile = libssh2_sftp_open(sftp_session_, remoteFilePath.c_str(),
                                                       LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
                                                       LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR |
                                                       LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
    if (!remoteFile) {
        result.error_message = "Cannot open remote file: " + remoteFilePath + " - " + getLibssh2Error();
        return result;
    }
    
    // Transfer file in chunks
    const size_t bufferSize = 8192;
    char buffer[bufferSize];
    size_t totalTransferred = 0;
    
    while (!localFile.eof()) {
        localFile.read(buffer, bufferSize);
        size_t bytesRead = localFile.gcount();
        
        if (bytesRead > 0) {
            ssize_t bytesWritten = libssh2_sftp_write(remoteFile, buffer, bytesRead);
            if (bytesWritten < 0) {
                result.error_message = "Write error: " + getLibssh2Error();
                libssh2_sftp_close(remoteFile);
                return result;
            }
            totalTransferred += bytesWritten;
        }
    }
    
    libssh2_sftp_close(remoteFile);
    localFile.close();
    
    auto end = std::chrono::high_resolution_clock::now();
    
    result.success = true;
    result.bytes_transferred = totalTransferred;
    result.transfer_time = std::chrono::duration<double>(end - start).count();
    
    return result;
}

SftpTransferResult SftpClient::downloadFile(const std::string& remoteFilePath, const std::string& localFilePath) {
    SftpTransferResult result;
    result.success = false;
    result.local_path = localFilePath;
    result.remote_path = remoteFilePath;
    
    if (!connected_) {
        result.error_message = "Not connected to SFTP server";
        return result;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Open remote file for reading
    LIBSSH2_SFTP_HANDLE* remoteFile = libssh2_sftp_open(sftp_session_, remoteFilePath.c_str(),
                                                       LIBSSH2_FXF_READ, 0);
    if (!remoteFile) {
        result.error_message = "Cannot open remote file: " + remoteFilePath + " - " + getLibssh2Error();
        return result;
    }
    
    // Create local file
    std::ofstream localFile(localFilePath, std::ios::binary);
    if (!localFile.is_open()) {
        result.error_message = "Cannot create local file: " + localFilePath;
        libssh2_sftp_close(remoteFile);
        return result;
    }
    
    // Transfer file in chunks
    const size_t bufferSize = 8192;
    char buffer[bufferSize];
    size_t totalTransferred = 0;
    
    while (true) {
        ssize_t bytesRead = libssh2_sftp_read(remoteFile, buffer, bufferSize);
        if (bytesRead < 0) {
            result.error_message = "Read error: " + getLibssh2Error();
            libssh2_sftp_close(remoteFile);
            localFile.close();
            return result;
        }
        
        if (bytesRead == 0) {
            break; // End of file
        }
        
        localFile.write(buffer, bytesRead);
        totalTransferred += bytesRead;
    }
    
    libssh2_sftp_close(remoteFile);
    localFile.close();
    
    auto end = std::chrono::high_resolution_clock::now();
    
    result.success = true;
    result.bytes_transferred = totalTransferred;
    result.transfer_time = std::chrono::duration<double>(end - start).count();
    
    return result;
}

std::vector<SftpFileInfo> SftpClient::listDirectory(const std::string& remotePath) {
    std::vector<SftpFileInfo> files;
    
    if (!connected_) {
        last_error_ = "Not connected to SFTP server";
        return files;
    }
    
    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_opendir(sftp_session_, remotePath.c_str());
    if (!handle) {
        last_error_ = "Cannot open directory: " + remotePath + " - " + getLibssh2Error();
        return files;
    }
    
    char buffer[512];
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    
    while (true) {
        int rc = libssh2_sftp_readdir(handle, buffer, sizeof(buffer), &attrs);
        if (rc <= 0) {
            break;
        }
        
        std::string filename(buffer, rc);
        
        // Skip . and ..
        if (filename == "." || filename == "..") {
            continue;
        }
        
        SftpFileInfo fileInfo;
        fileInfo.name = filename;
        fileInfo.path = remotePath + "/" + filename;
        fileInfo.is_directory = (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) && 
                               LIBSSH2_SFTP_S_ISDIR(attrs.permissions);
        fileInfo.size = (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) ? attrs.filesize : 0;
        fileInfo.permissions = (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) ? attrs.permissions : 0;
        fileInfo.modified_time = (attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME) ? attrs.mtime : 0;
        fileInfo.access_time = (attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME) ? attrs.atime : 0;
        
        files.push_back(fileInfo);
    }
    
    libssh2_sftp_closedir(handle);
    return files;
}

std::vector<std::string> SftpClient::listFiles(const std::string& remotePath, bool recursive) {
    std::vector<std::string> files;
    auto fileInfos = listDirectory(remotePath);
    
    for (const auto& fileInfo : fileInfos) {
        if (!fileInfo.is_directory) {
            files.push_back(fileInfo.path);
        } else if (recursive) {
            auto subFiles = listFiles(fileInfo.path, true);
            files.insert(files.end(), subFiles.begin(), subFiles.end());
        }
    }
    
    return files;
}

bool SftpClient::deleteFile(const std::string& remoteFilePath) {
    if (!connected_) {
        last_error_ = "Not connected to SFTP server";
        return false;
    }
    
    int rc = libssh2_sftp_unlink(sftp_session_, remoteFilePath.c_str());
    if (rc != 0) {
        last_error_ = "Failed to delete file: " + remoteFilePath + " - " + getLibssh2Error();
        return false;
    }
    
    return true;
}

bool SftpClient::fileExists(const std::string& remoteFilePath) {
    if (!connected_) {
        return false;
    }
    
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int rc = libssh2_sftp_stat(sftp_session_, remoteFilePath.c_str(), &attrs);
    return (rc == 0);
}

SftpClient::BatchTransferResult SftpClient::uploadDirectory(const std::string& localDirectory, 
                                                           const std::string& remoteDirectory,
                                                           const std::string& filePattern,
                                                           bool recursive) {
    BatchTransferResult result;
    result.successful_transfers = 0;
    result.failed_transfers = 0;
    result.total_bytes = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto files = getFilesInLocalDirectory(localDirectory, filePattern, recursive);
    
    for (const auto& localFile : files) {
        std::filesystem::path localPath(localFile);
        std::filesystem::path relativePath = std::filesystem::relative(localPath, localDirectory);
        std::string remoteFile = remoteDirectory + "/" + relativePath.string();
        
        // Create remote directory if needed
        std::filesystem::path remoteParent = std::filesystem::path(remoteFile).parent_path();
        createDirectoryRecursive(remoteParent.string());
        
        auto transferResult = uploadFile(localFile, remoteFile);
        
        if (transferResult.success) {
            result.successful_transfers++;
            result.total_bytes += transferResult.bytes_transferred;
        } else {
            result.failed_transfers++;
            result.failed_files.push_back(localFile);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.total_time = std::chrono::duration<double>(end - start).count();
    
    return result;
}

std::vector<std::string> SftpClient::getFilesInLocalDirectory(const std::string& directory, 
                                                             const std::string& pattern,
                                                             bool recursive) {
    std::vector<std::string> files;
    
    try {
        if (recursive) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    if (matchesPattern(entry.path().filename().string(), pattern)) {
                        files.push_back(entry.path().string());
                    }
                }
            }
        } else {
            for (const auto& entry : std::filesystem::directory_iterator(directory)) {
                if (entry.is_regular_file()) {
                    if (matchesPattern(entry.path().filename().string(), pattern)) {
                        files.push_back(entry.path().string());
                    }
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& ex) {
        last_error_ = "Error accessing directory: " + std::string(ex.what());
    }
    
    return files;
}

bool SftpClient::matchesPattern(const std::string& filename, const std::string& pattern) {
    if (pattern == "*") {
        return true;
    }
    
    // Simple pattern matching - in a real implementation, you'd use proper regex
    return filename.find(pattern) != std::string::npos;
}

bool SftpClient::createDirectoryRecursive(const std::string& remotePath) {
    if (!connected_) {
        return false;
    }
    
    // Try to create the directory
    int rc = libssh2_sftp_mkdir(sftp_session_, remotePath.c_str(),
                               LIBSSH2_SFTP_S_IRWXU | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
    
    if (rc == 0) {
        return true; // Successfully created
    }
    
    // If it failed, try to create parent directories first
    std::filesystem::path path(remotePath);
    std::filesystem::path parent = path.parent_path();
    
    if (!parent.empty() && parent != path) {
        if (createDirectoryRecursive(parent.string())) {
            // Try again after creating parent
            rc = libssh2_sftp_mkdir(sftp_session_, remotePath.c_str(),
                                   LIBSSH2_SFTP_S_IRWXU | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
            return (rc == 0);
        }
    }
    
    return false;
}

std::string SftpClient::getLastError() const {
    return last_error_;
}

void SftpClient::setTimeout(int timeoutSeconds) {
    timeout_seconds_ = timeoutSeconds;
    if (session_) {
        libssh2_session_set_timeout(session_, timeout_seconds_ * 1000);
    }
}

} // namespace etl
