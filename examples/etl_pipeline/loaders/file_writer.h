#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <functional>

namespace etl {

struct LoadResult {
    bool success;
    std::string error_message;
    size_t records_processed;
    size_t bytes_written;
    double processing_time;
    std::string output_location;
};

enum class OutputFormat {
    JSON,
    CSV,
    XML,
    PARQUET,
    BINARY
};

struct FileWriterConfig {
    OutputFormat format;
    std::string output_directory;
    std::string filename_prefix;
    std::string filename_suffix;
    bool append_timestamp;
    bool compress_output;
    size_t max_file_size_mb;
    bool create_directories;
    std::map<std::string, std::string> custom_headers;
};

class FileWriter {
public:
    FileWriter();
    ~FileWriter();

    // Configuration
    void setConfig(const FileWriterConfig& config);
    void setOutputDirectory(const std::string& directory);
    void setOutputFormat(OutputFormat format);
    void setCompressionEnabled(bool enabled);
    void setMaxFileSize(size_t maxSizeMB);
    
    // Basic file operations
    LoadResult writeData(const std::string& data, const std::string& filename = "");
    LoadResult writeDataBatch(const std::vector<std::string>& dataItems, const std::string& filename = "");
    LoadResult appendData(const std::string& data, const std::string& filename);
    
    // Format-specific operations
    LoadResult writeJson(const std::string& jsonData, const std::string& filename = "");
    LoadResult writeJsonArray(const std::vector<std::string>& jsonObjects, const std::string& filename = "");
    LoadResult writeCsv(const std::string& csvData, const std::string& filename = "", bool includeHeaders = true);
    LoadResult writeCsvFromJson(const std::string& jsonData, const std::string& filename = "");
    
    // Streaming operations for large datasets
    class StreamWriter {
    public:
        StreamWriter(const std::string& filepath, OutputFormat format);
        ~StreamWriter();
        
        bool writeRecord(const std::string& record);
        bool writeHeader(const std::vector<std::string>& headers);
        void flush();
        void close();
        
        size_t getRecordCount() const;
        size_t getBytesWritten() const;
        
    private:
        std::unique_ptr<std::ofstream> file_stream_;
        OutputFormat format_;
        size_t record_count_;
        size_t bytes_written_;
        bool header_written_;
        bool is_first_record_;
    };
    
    std::unique_ptr<StreamWriter> createStreamWriter(const std::string& filename = "");
    
    // Partitioned output for large datasets
    struct PartitionConfig {
        std::string partition_field;
        size_t max_records_per_partition;
        size_t max_size_per_partition_mb;
        std::string partition_format; // "YYYY/MM/DD", "YYYY-MM", etc.
    };
    
    LoadResult writePartitionedData(const std::string& jsonArrayData, 
                                   const PartitionConfig& partitionConfig);
    
    // Batch operations
    struct BatchWriteResult {
        int successful_writes;
        int failed_writes;
        std::vector<std::string> failed_files;
        size_t total_bytes;
        double total_time;
    };
    
    BatchWriteResult writeMultipleFiles(const std::map<std::string, std::string>& fileDataMap);
    
    // Utility methods
    std::string generateFilename(const std::string& prefix = "", const std::string& suffix = "");
    std::vector<std::string> listOutputFiles(const std::string& pattern = "*");
    bool fileExists(const std::string& filepath);
    size_t getFileSize(const std::string& filepath);
    bool deleteFile(const std::string& filepath);
    
    // Compression utilities
    std::string compressData(const std::string& data, const std::string& algorithm = "gzip");
    std::string decompressData(const std::string& compressedData, const std::string& algorithm = "gzip");
    
    // Validation
    bool validateOutputPath(const std::string& path);
    LoadResult validateAndPrepareOutput(const std::string& data, const std::string& filename);
    
    // Statistics and monitoring
    struct WriterStats {
        size_t total_files_written;
        size_t total_bytes_written;
        size_t total_records_written;
        double total_processing_time;
        std::map<OutputFormat, size_t> format_distribution;
    };
    
    WriterStats getStatistics() const;
    void resetStatistics();
    
private:
    FileWriterConfig config_;
    WriterStats stats_;
    
    // Helper methods
    std::string formatDataForOutput(const std::string& data, OutputFormat format);
    std::string getFileExtension(OutputFormat format);
    bool createDirectoryIfNotExists(const std::string& path);
    std::string addTimestampToFilename(const std::string& filename);
    bool shouldRotateFile(const std::string& filepath);
    std::string getNextRotatedFilename(const std::string& baseFilename);
    
    // Format converters
    std::string jsonToCsv(const std::string& jsonData);
    std::string jsonToXml(const std::string& jsonData);
    std::string csvToJson(const std::string& csvData);
};

} // namespace etl
