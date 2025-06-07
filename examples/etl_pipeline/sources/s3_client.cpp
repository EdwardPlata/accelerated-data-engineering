#include "s3_client.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <aws/core/utils/Outcome.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/HeadBucketRequest.h>

namespace etl {

S3Client::S3Client() : initialized_(false) {
    Aws::InitAPI(options_);
}

S3Client::~S3Client() {
    Aws::ShutdownAPI(options_);
}

bool S3Client::initialize(const std::string& region) {
    region_ = region;
    
    Aws::Client::ClientConfiguration config;
    config.region = region_;
    
    s3_client_ = std::make_unique<Aws::S3::S3Client>(config);
    initialized_ = true;
    
    return true;
}

void S3Client::setCredentials(const std::string& accessKeyId, const std::string& secretAccessKey, 
                             const std::string& sessionToken) {
    Aws::Auth::AWSCredentials credentials(accessKeyId, secretAccessKey, sessionToken);
    
    Aws::Client::ClientConfiguration config;
    config.region = region_;
    
    s3_client_ = std::make_unique<Aws::S3::S3Client>(credentials, config);
}

void S3Client::setBucket(const std::string& bucketName) {
    bucket_name_ = bucketName;
}

void S3Client::setEndpointUrl(const std::string& endpointUrl) {
    Aws::Client::ClientConfiguration config;
    config.region = region_;
    config.endpointOverride = endpointUrl;
    
    s3_client_ = std::make_unique<Aws::S3::S3Client>(config);
}

S3UploadResult S3Client::uploadFile(const std::string& localFilePath, const std::string& s3Key) {
    S3UploadResult result;
    result.success = false;
    
    if (!initialized_ || bucket_name_.empty()) {
        result.error_message = "S3Client not properly initialized or bucket not set";
        return result;
    }
    
    std::ifstream file(localFilePath, std::ios::binary);
    if (!file.is_open()) {
        result.error_message = "Cannot open local file: " + localFilePath;
        return result;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Read file content
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    auto streamBuf = Aws::MakeShared<Aws::StringStream>("S3Upload");
    streamBuf->write(reinterpret_cast<char*>(file.rdbuf()), fileSize);
    
    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(bucket_name_);
    request.SetKey(s3Key);
    request.SetBody(streamBuf);
    request.SetContentType(inferContentType(localFilePath));
    request.SetContentLength(fileSize);
    
    auto outcome = s3_client_->PutObject(request);
    auto end = std::chrono::high_resolution_clock::now();
    
    result.upload_time = std::chrono::duration<double>(end - start).count();
    result.bytes_transferred = fileSize;
    
    if (outcome.IsSuccess()) {
        result.success = true;
        result.etag = outcome.GetResult().GetETag();
    } else {
        result.error_message = outcome.GetError().GetMessage();
    }
    
    return result;
}

S3UploadResult S3Client::uploadData(const std::string& data, const std::string& s3Key, 
                                   const std::string& contentType) {
    S3UploadResult result;
    result.success = false;
    
    if (!initialized_ || bucket_name_.empty()) {
        result.error_message = "S3Client not properly initialized or bucket not set";
        return result;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto streamBuf = Aws::MakeShared<Aws::StringStream>("S3Upload");
    streamBuf->str(data);
    
    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(bucket_name_);
    request.SetKey(s3Key);
    request.SetBody(streamBuf);
    request.SetContentType(contentType);
    request.SetContentLength(data.length());
    
    auto outcome = s3_client_->PutObject(request);
    auto end = std::chrono::high_resolution_clock::now();
    
    result.upload_time = std::chrono::duration<double>(end - start).count();
    result.bytes_transferred = data.length();
    
    if (outcome.IsSuccess()) {
        result.success = true;
        result.etag = outcome.GetResult().GetETag();
    } else {
        result.error_message = outcome.GetError().GetMessage();
    }
    
    return result;
}

S3DownloadResult S3Client::downloadFile(const std::string& s3Key, const std::string& localFilePath) {
    S3DownloadResult result;
    result.success = false;
    
    if (!initialized_ || bucket_name_.empty()) {
        result.error_message = "S3Client not properly initialized or bucket not set";
        return result;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket(bucket_name_);
    request.SetKey(s3Key);
    
    auto outcome = s3_client_->GetObject(request);
    auto end = std::chrono::high_resolution_clock::now();
    
    result.download_time = std::chrono::duration<double>(end - start).count();
    
    if (outcome.IsSuccess()) {
        auto& body = outcome.GetResult().GetBody();
        std::ofstream outFile(localFilePath, std::ios::binary);
        
        if (outFile.is_open()) {
            outFile << body.rdbuf();
            outFile.close();
            
            result.success = true;
            result.bytes_transferred = std::filesystem::file_size(localFilePath);
            
            // Get metadata
            auto metadata = outcome.GetResult().GetMetadata();
            for (const auto& pair : metadata) {
                result.metadata[pair.first] = pair.second;
            }
        } else {
            result.error_message = "Cannot create local file: " + localFilePath;
        }
    } else {
        result.error_message = outcome.GetError().GetMessage();
    }
    
    return result;
}

S3DownloadResult S3Client::downloadToMemory(const std::string& s3Key) {
    S3DownloadResult result;
    result.success = false;
    
    if (!initialized_ || bucket_name_.empty()) {
        result.error_message = "S3Client not properly initialized or bucket not set";
        return result;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket(bucket_name_);
    request.SetKey(s3Key);
    
    auto outcome = s3_client_->GetObject(request);
    auto end = std::chrono::high_resolution_clock::now();
    
    result.download_time = std::chrono::duration<double>(end - start).count();
    
    if (outcome.IsSuccess()) {
        auto& body = outcome.GetResult().GetBody();
        std::stringstream ss;
        ss << body.rdbuf();
        result.content = ss.str();
        result.bytes_transferred = result.content.length();
        result.success = true;
        
        // Get metadata
        auto metadata = outcome.GetResult().GetMetadata();
        for (const auto& pair : metadata) {
            result.metadata[pair.first] = pair.second;
        }
    } else {
        result.error_message = outcome.GetError().GetMessage();
    }
    
    return result;
}

std::vector<S3Object> S3Client::listObjects(const std::string& prefix, int maxKeys) {
    std::vector<S3Object> objects;
    
    if (!initialized_ || bucket_name_.empty()) {
        return objects;
    }
    
    Aws::S3::Model::ListObjectsV2Request request;
    request.SetBucket(bucket_name_);
    if (!prefix.empty()) {
        request.SetPrefix(prefix);
    }
    request.SetMaxKeys(maxKeys);
    
    auto outcome = s3_client_->ListObjectsV2(request);
    
    if (outcome.IsSuccess()) {
        const auto& result = outcome.GetResult();
        for (const auto& object : result.GetContents()) {
            S3Object s3obj;
            s3obj.key = object.GetKey();
            s3obj.etag = object.GetETag();
            s3obj.size = object.GetSize();
            s3obj.last_modified = object.GetLastModified().ToGmtString(Aws::Utils::DateFormat::ISO_8601);
            s3obj.storage_class = Aws::S3::Model::StorageClassMapper::GetNameForStorageClass(object.GetStorageClass());
            objects.push_back(s3obj);
        }
    }
    
    return objects;
}

std::vector<std::string> S3Client::listObjectKeys(const std::string& prefix, int maxKeys) {
    std::vector<std::string> keys;
    auto objects = listObjects(prefix, maxKeys);
    
    for (const auto& obj : objects) {
        keys.push_back(obj.key);
    }
    
    return keys;
}

bool S3Client::deleteObject(const std::string& s3Key) {
    if (!initialized_ || bucket_name_.empty()) {
        return false;
    }
    
    Aws::S3::Model::DeleteObjectRequest request;
    request.SetBucket(bucket_name_);
    request.SetKey(s3Key);
    
    auto outcome = s3_client_->DeleteObject(request);
    return outcome.IsSuccess();
}

bool S3Client::objectExists(const std::string& s3Key) {
    if (!initialized_ || bucket_name_.empty()) {
        return false;
    }
    
    Aws::S3::Model::HeadObjectRequest request;
    request.SetBucket(bucket_name_);
    request.SetKey(s3Key);
    
    auto outcome = s3_client_->HeadObject(request);
    return outcome.IsSuccess();
}

S3Object S3Client::getObjectInfo(const std::string& s3Key) {
    S3Object obj;
    obj.key = s3Key;
    
    if (!initialized_ || bucket_name_.empty()) {
        return obj;
    }
    
    Aws::S3::Model::HeadObjectRequest request;
    request.SetBucket(bucket_name_);
    request.SetKey(s3Key);
    
    auto outcome = s3_client_->HeadObject(request);
    
    if (outcome.IsSuccess()) {
        const auto& result = outcome.GetResult();
        obj.etag = result.GetETag();
        obj.size = result.GetContentLength();
        obj.last_modified = result.GetLastModified().ToGmtString(Aws::Utils::DateFormat::ISO_8601);
    }
    
    return obj;
}

S3Client::BatchUploadResult S3Client::uploadDirectory(const std::string& localDirectory, 
                                                     const std::string& s3Prefix,
                                                     const std::string& filePattern) {
    BatchUploadResult result;
    result.successful_uploads = 0;
    result.failed_uploads = 0;
    result.total_bytes = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto files = getFilesInDirectory(localDirectory, filePattern);
    
    for (const auto& filePath : files) {
        std::filesystem::path path(filePath);
        std::string fileName = path.filename().string();
        std::string s3Key = s3Prefix.empty() ? fileName : s3Prefix + "/" + fileName;
        
        auto uploadResult = uploadFile(filePath, s3Key);
        
        if (uploadResult.success) {
            result.successful_uploads++;
            result.total_bytes += uploadResult.bytes_transferred;
        } else {
            result.failed_uploads++;
            result.failed_files.push_back(filePath);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.total_time = std::chrono::duration<double>(end - start).count();
    
    return result;
}

S3Client::BatchDownloadResult S3Client::downloadObjects(const std::vector<std::string>& s3Keys, 
                                                       const std::string& localDirectory) {
    BatchDownloadResult result;
    result.successful_downloads = 0;
    result.failed_downloads = 0;
    result.total_bytes = 0;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Create directory if it doesn't exist
    std::filesystem::create_directories(localDirectory);
    
    for (const auto& s3Key : s3Keys) {
        std::filesystem::path keyPath(s3Key);
        std::string fileName = keyPath.filename().string();
        std::string localPath = localDirectory + "/" + fileName;
        
        auto downloadResult = downloadFile(s3Key, localPath);
        
        if (downloadResult.success) {
            result.successful_downloads++;
            result.total_bytes += downloadResult.bytes_transferred;
        } else {
            result.failed_downloads++;
            result.failed_keys.push_back(s3Key);
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.total_time = std::chrono::duration<double>(end - start).count();
    
    return result;
}

std::string S3Client::inferContentType(const std::string& filename) {
    std::string ext = getFileExtension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".png") return "image/png";
    if (ext == ".gif") return "image/gif";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".txt") return "text/plain";
    if (ext == ".csv") return "text/csv";
    if (ext == ".json") return "application/json";
    if (ext == ".xml") return "application/xml";
    if (ext == ".zip") return "application/zip";
    if (ext == ".tar") return "application/x-tar";
    if (ext == ".gz") return "application/gzip";
    
    return "application/octet-stream";
}

std::string S3Client::getFileExtension(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos && pos < filename.length() - 1) {
        return filename.substr(pos);
    }
    return "";
}

size_t S3Client::getFileSize(const std::string& filePath) {
    try {
        return std::filesystem::file_size(filePath);
    } catch (const std::filesystem::filesystem_error&) {
        return 0;
    }
}

std::vector<std::string> S3Client::getFilesInDirectory(const std::string& directory, 
                                                      const std::string& pattern) {
    std::vector<std::string> files;
    
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
            if (entry.is_regular_file()) {
                // Simple pattern matching - in a real implementation, you'd use proper regex
                if (pattern == "*" || entry.path().filename().string().find(pattern) != std::string::npos) {
                    files.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& ex) {
        std::cerr << "Error accessing directory: " << ex.what() << std::endl;
    }
    
    return files;
}

} // namespace etl
