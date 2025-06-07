#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/core/auth/AWSCredentialsProvider.h>

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
    S3Client();
    ~S3Client();

    // Configuration
    bool initialize(const std::string& region = "us-east-1");
    void setCredentials(const std::string& accessKeyId, const std::string& secretAccessKey, 
                       const std::string& sessionToken = "");
    void setBucket(const std::string& bucketName);
    void setEndpointUrl(const std::string& endpointUrl);
    
    // Upload operations
    S3UploadResult uploadFile(const std::string& localFilePath, const std::string& s3Key);
    S3UploadResult uploadData(const std::string& data, const std::string& s3Key, 
                             const std::string& contentType = "application/octet-stream");
    S3UploadResult uploadFileWithProgress(const std::string& localFilePath, const std::string& s3Key,
                                         const std::function<void(size_t, size_t)>& progressCallback = nullptr);
    
    // Download operations
    S3DownloadResult downloadFile(const std::string& s3Key, const std::string& localFilePath);
    S3DownloadResult downloadToMemory(const std::string& s3Key);
    S3DownloadResult downloadWithProgress(const std::string& s3Key, const std::string& localFilePath,
                                         const std::function<void(size_t, size_t)>& progressCallback = nullptr);
    
    // List operations
    std::vector<S3Object> listObjects(const std::string& prefix = "", int maxKeys = 1000);
    std::vector<std::string> listObjectKeys(const std::string& prefix = "", int maxKeys = 1000);
    
    // Management operations
    bool deleteObject(const std::string& s3Key);
    bool deleteObjects(const std::vector<std::string>& s3Keys);
    bool objectExists(const std::string& s3Key);
    S3Object getObjectInfo(const std::string& s3Key);
    
    // Utility methods
    bool createBucket(const std::string& bucketName, const std::string& region = "");
    bool deleteBucket(const std::string& bucketName);
    bool bucketExists(const std::string& bucketName);
    
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
    std::unique_ptr<Aws::S3::S3Client> s3_client_;
    std::string bucket_name_;
    std::string region_;
    Aws::SDKOptions options_;
    bool initialized_;
    
    // Helper methods
    std::string getFileExtension(const std::string& filename);
    std::string inferContentType(const std::string& filename);
    size_t getFileSize(const std::string& filePath);
    std::vector<std::string> getFilesInDirectory(const std::string& directory, 
                                                const std::string& pattern = "*");
};

} // namespace etl
