#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <chrono>

// Simplified S3 client for demonstration (without AWS SDK dependency)

namespace etl {

struct S3Object {
    std::string key;
    std::string etag;
    long long size;
    std::string last_modified;
    std::string storage_class;
};

struct S3UploadResult {
    bool success;
    std::string error_message;
    std::string etag;
    std::string location;
    size_t bytes_transferred;
    double upload_time;
};

struct S3DownloadResult {
    bool success;
    std::string error_message;
    std::string content;
    size_t bytes_transferred;
    double download_time;
    std::map<std::string, std::string> metadata;
};

class S3Client {
public:
    S3Client(const std::string& bucket, const std::string& region,
             const std::string& accessKey, const std::string& secretKey);
    ~S3Client();

    // Configuration
    void setBucket(const std::string& bucketName);
    void setEndpointUrl(const std::string& endpointUrl);
    
    // Upload operations (simulated)
    S3UploadResult uploadFile(const std::string& localFilePath, const std::string& s3Key);
    S3UploadResult uploadData(const std::string& data, const std::string& s3Key, 
                             const std::string& contentType = "application/octet-stream");
    
    // Download operations (simulated)
    S3DownloadResult downloadFile(const std::string& s3Key, const std::string& localFilePath);
    S3DownloadResult downloadToMemory(const std::string& s3Key);
    
    // List operations (simulated)
    std::vector<S3Object> listObjects(const std::string& prefix = "", int maxKeys = 1000);
    std::vector<std::string> listObjectKeys(const std::string& prefix = "", int maxKeys = 1000);
    
    // Management operations (simulated)
    bool deleteObject(const std::string& s3Key);
    bool deleteObjects(const std::vector<std::string>& s3Keys);
    bool objectExists(const std::string& s3Key);
    S3Object getObjectInfo(const std::string& s3Key);
    
    // Batch operations for ETL
    struct BatchUploadResult {
        int successful_uploads;
        int failed_uploads;
        std::vector<std::string> failed_files;
        double total_time;
        size_t total_bytes;
    };
    
    BatchUploadResult uploadDirectory(const std::string& localDirectory, 
                                     const std::string& s3Prefix = "",
                                     const std::string& filePattern = "*");
    
    struct BatchDownloadResult {
        int successful_downloads;
        int failed_downloads;
        std::vector<std::string> failed_keys;
        double total_time;
        size_t total_bytes;
    };
    
    BatchDownloadResult downloadObjects(const std::vector<std::string>& s3Keys, 
                                       const std::string& localDirectory);

private:
    std::string bucket_name_;
    std::string region_;
    std::string access_key_;
    std::string secret_key_;
    std::string endpoint_url_;
    bool initialized_;
    
    // Helper methods
    std::string getFileExtension(const std::string& filename);
    std::string inferContentType(const std::string& filename);
    size_t getFileSize(const std::string& filePath);
    std::vector<std::string> getFilesInDirectory(const std::string& directory, 
                                                const std::string& pattern = "*");
    
    // Simulation helpers
    void simulateOperation(const std::string& operation, const std::string& key);
};

} // namespace etl
