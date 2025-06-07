#include "file_writer.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <regex>
#include <nlohmann/json.hpp>

namespace etl {

FileWriter::FileWriter() {
    // Set default configuration
    config_.format = OutputFormat::JSON;
    config_.output_directory = "./output";
    config_.filename_prefix = "etl_output";
    config_.filename_suffix = "";
    config_.append_timestamp = true;
    config_.compress_output = false;
    config_.max_file_size_mb = 100;
    config_.create_directories = true;
    
    resetStatistics();
}

FileWriter::~FileWriter() {}

void FileWriter::setConfig(const FileWriterConfig& config) {
    config_ = config;
}

void FileWriter::setOutputDirectory(const std::string& directory) {
    config_.output_directory = directory;
}

void FileWriter::setOutputFormat(OutputFormat format) {
    config_.format = format;
}

void FileWriter::setCompressionEnabled(bool enabled) {
    config_.compress_output = enabled;
}

void FileWriter::setMaxFileSize(size_t maxSizeMB) {
    config_.max_file_size_mb = maxSizeMB;
}

LoadResult FileWriter::writeData(const std::string& data, const std::string& filename) {
    LoadResult result;
    result.success = false;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // Generate filename if not provided
        std::string outputFilename = filename.empty() ? generateFilename() : filename;
        std::string fullPath = config_.output_directory + "/" + outputFilename;
        
        // Create directories if needed
        if (config_.create_directories) {
            std::filesystem::create_directories(config_.output_directory);
        }
        
        // Validate output path
        if (!validateOutputPath(fullPath)) {
            result.error_message = "Invalid output path: " + fullPath;
            return result;
        }
        
        // Format data according to specified format
        std::string formattedData = formatDataForOutput(data, config_.format);
        
        // Compress if enabled
        if (config_.compress_output) {
            formattedData = compressData(formattedData);
            if (fullPath.find(".gz") == std::string::npos) {
                fullPath += ".gz";
            }
        }
        
        // Write to file
        std::ofstream outFile(fullPath, std::ios::binary);
        if (!outFile.is_open()) {
            result.error_message = "Cannot open file for writing: " + fullPath;
            return result;
        }
        
        outFile.write(formattedData.c_str(), formattedData.length());
        outFile.close();
        
        if (outFile.fail()) {
            result.error_message = "Error writing to file: " + fullPath;
            return result;
        }
        
        result.success = true;
        result.bytes_written = formattedData.length();
        result.records_processed = 1;
        result.output_location = fullPath;
        
        // Update statistics
        stats_.total_files_written++;
        stats_.total_bytes_written += result.bytes_written;
        stats_.total_records_written += result.records_processed;
        stats_.format_distribution[config_.format]++;
        
    } catch (const std::exception& e) {
        result.error_message = e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.processing_time = std::chrono::duration<double>(end - start).count();
    stats_.total_processing_time += result.processing_time;
    
    return result;
}

LoadResult FileWriter::writeDataBatch(const std::vector<std::string>& dataItems, const std::string& filename) {
    LoadResult result;
    result.success = false;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        if (dataItems.empty()) {
            result.error_message = "No data items to write";
            return result;
        }
        
        std::string outputFilename = filename.empty() ? generateFilename() : filename;
        std::string fullPath = config_.output_directory + "/" + outputFilename;
        
        if (config_.create_directories) {
            std::filesystem::create_directories(config_.output_directory);
        }
        
        std::ofstream outFile(fullPath);
        if (!outFile.is_open()) {
            result.error_message = "Cannot open file for writing: " + fullPath;
            return result;
        }
        
        size_t totalBytes = 0;
        size_t recordsProcessed = 0;
        
        // Handle different formats
        if (config_.format == OutputFormat::JSON) {
            outFile << "[\n";
            for (size_t i = 0; i < dataItems.size(); ++i) {
                if (i > 0) outFile << ",\n";
                
                std::string formattedItem = formatDataForOutput(dataItems[i], config_.format);
                outFile << "  " << formattedItem;
                totalBytes += formattedItem.length();
                recordsProcessed++;
            }
            outFile << "\n]";
            totalBytes += 4; // For brackets and newlines
            
        } else if (config_.format == OutputFormat::CSV) {
            // For CSV, assume first item contains headers or write all items as rows
            for (const auto& item : dataItems) {
                std::string formattedItem = formatDataForOutput(item, config_.format);
                outFile << formattedItem << "\n";
                totalBytes += formattedItem.length() + 1;
                recordsProcessed++;
            }
            
        } else {
            // For other formats, write items sequentially
            for (const auto& item : dataItems) {
                std::string formattedItem = formatDataForOutput(item, config_.format);
                outFile << formattedItem << "\n";
                totalBytes += formattedItem.length() + 1;
                recordsProcessed++;
            }
        }
        
        outFile.close();
        
        result.success = true;
        result.bytes_written = totalBytes;
        result.records_processed = recordsProcessed;
        result.output_location = fullPath;
        
        // Update statistics
        stats_.total_files_written++;
        stats_.total_bytes_written += result.bytes_written;
        stats_.total_records_written += result.records_processed;
        stats_.format_distribution[config_.format]++;
        
    } catch (const std::exception& e) {
        result.error_message = e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.processing_time = std::chrono::duration<double>(end - start).count();
    stats_.total_processing_time += result.processing_time;
    
    return result;
}

LoadResult FileWriter::writeJson(const std::string& jsonData, const std::string& filename) {
    // Validate JSON before writing
    try {
        nlohmann::json::parse(jsonData);
    } catch (const std::exception& e) {
        LoadResult result;
        result.success = false;
        result.error_message = "Invalid JSON data: " + std::string(e.what());
        return result;
    }
    
    OutputFormat originalFormat = config_.format;
    config_.format = OutputFormat::JSON;
    
    LoadResult result = writeData(jsonData, filename);
    
    config_.format = originalFormat;
    return result;
}

LoadResult FileWriter::writeCsvFromJson(const std::string& jsonData, const std::string& filename) {
    try {
        std::string csvData = jsonToCsv(jsonData);
        
        OutputFormat originalFormat = config_.format;
        config_.format = OutputFormat::CSV;
        
        LoadResult result = writeData(csvData, filename);
        
        config_.format = originalFormat;
        return result;
        
    } catch (const std::exception& e) {
        LoadResult result;
        result.success = false;
        result.error_message = "Error converting JSON to CSV: " + std::string(e.what());
        return result;
    }
}

// StreamWriter implementation
FileWriter::StreamWriter::StreamWriter(const std::string& filepath, OutputFormat format) 
    : format_(format), record_count_(0), bytes_written_(0), header_written_(false), is_first_record_(true) {
    
    file_stream_ = std::make_unique<std::ofstream>(filepath);
    
    if (format_ == OutputFormat::JSON) {
        *file_stream_ << "[\n";
        bytes_written_ += 2;
    }
}

FileWriter::StreamWriter::~StreamWriter() {
    close();
}

bool FileWriter::StreamWriter::writeRecord(const std::string& record) {
    if (!file_stream_ || !file_stream_->is_open()) {
        return false;
    }
    
    if (format_ == OutputFormat::JSON) {
        if (!is_first_record_) {
            *file_stream_ << ",\n";
            bytes_written_ += 2;
        }
        *file_stream_ << "  " << record;
        bytes_written_ += record.length() + 2;
        
    } else if (format_ == OutputFormat::CSV) {
        *file_stream_ << record << "\n";
        bytes_written_ += record.length() + 1;
        
    } else {
        *file_stream_ << record << "\n";
        bytes_written_ += record.length() + 1;
    }
    
    is_first_record_ = false;
    record_count_++;
    
    return true;
}

void FileWriter::StreamWriter::close() {
    if (file_stream_ && file_stream_->is_open()) {
        if (format_ == OutputFormat::JSON) {
            *file_stream_ << "\n]";
            bytes_written_ += 2;
        }
        file_stream_->close();
    }
}

std::unique_ptr<FileWriter::StreamWriter> FileWriter::createStreamWriter(const std::string& filename) {
    std::string outputFilename = filename.empty() ? generateFilename() : filename;
    std::string fullPath = config_.output_directory + "/" + outputFilename;
    
    if (config_.create_directories) {
        std::filesystem::create_directories(config_.output_directory);
    }
    
    return std::make_unique<StreamWriter>(fullPath, config_.format);
}

std::string FileWriter::generateFilename(const std::string& prefix, const std::string& suffix) {
    std::stringstream filename;
    
    // Use provided prefix or default
    filename << (prefix.empty() ? config_.filename_prefix : prefix);
    
    // Add timestamp if configured
    if (config_.append_timestamp) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        
        filename << "_" << std::put_time(&tm, "%Y%m%d_%H%M%S");
    }
    
    // Add suffix
    if (!suffix.empty()) {
        filename << "_" << suffix;
    } else if (!config_.filename_suffix.empty()) {
        filename << "_" << config_.filename_suffix;
    }
    
    // Add extension based on format
    filename << getFileExtension(config_.format);
    
    return filename.str();
}

std::string FileWriter::formatDataForOutput(const std::string& data, OutputFormat format) {
    switch (format) {
        case OutputFormat::JSON:
            // Validate and pretty-print JSON
            try {
                nlohmann::json jsonObj = nlohmann::json::parse(data);
                return jsonObj.dump(2);
            } catch (const std::exception&) {
                return data; // Return as-is if not valid JSON
            }
            
        case OutputFormat::CSV:
            // If data is JSON, convert to CSV
            if (data.front() == '{' || data.front() == '[') {
                return jsonToCsv(data);
            }
            return data;
            
        case OutputFormat::XML:
            // Convert JSON to XML if needed
            if (data.front() == '{' || data.front() == '[') {
                return jsonToXml(data);
            }
            return data;
            
        default:
            return data;
    }
}

std::string FileWriter::getFileExtension(OutputFormat format) {
    switch (format) {
        case OutputFormat::JSON: return ".json";
        case OutputFormat::CSV: return ".csv";
        case OutputFormat::XML: return ".xml";
        case OutputFormat::PARQUET: return ".parquet";
        case OutputFormat::BINARY: return ".bin";
        default: return ".txt";
    }
}

bool FileWriter::createDirectoryIfNotExists(const std::string& path) {
    try {
        std::filesystem::path dirPath(path);
        if (dirPath.has_filename()) {
            dirPath = dirPath.parent_path();
        }
        return std::filesystem::create_directories(dirPath);
    } catch (const std::exception&) {
        return false;
    }
}

bool FileWriter::validateOutputPath(const std::string& path) {
    try {
        std::filesystem::path filePath(path);
        std::filesystem::path dirPath = filePath.parent_path();
        
        // Check if directory exists or can be created
        if (!std::filesystem::exists(dirPath)) {
            if (config_.create_directories) {
                return createDirectoryIfNotExists(dirPath.string());
            } else {
                return false;
            }
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string FileWriter::jsonToCsv(const std::string& jsonData) {
    nlohmann::json data = nlohmann::json::parse(jsonData);
    std::stringstream csv;
    
    if (data.is_array() && !data.empty()) {
        // Get headers from first object
        std::vector<std::string> headers;
        if (data[0].is_object()) {
            for (auto it = data[0].begin(); it != data[0].end(); ++it) {
                headers.push_back(it.key());
            }
        }
        
        // Write headers
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i > 0) csv << ",";
            csv << headers[i];
        }
        csv << "\n";
        
        // Write data rows
        for (const auto& record : data) {
            if (record.is_object()) {
                for (size_t i = 0; i < headers.size(); ++i) {
                    if (i > 0) csv << ",";
                    
                    if (record.contains(headers[i])) {
                        std::string value = record[headers[i]].is_string() 
                            ? record[headers[i]].get<std::string>()
                            : record[headers[i]].dump();
                        
                        // Escape CSV field if needed
                        if (value.find(',') != std::string::npos || 
                            value.find('"') != std::string::npos) {
                            std::string escaped = "\"";
                            for (char c : value) {
                                if (c == '"') escaped += "\"\"";
                                else escaped += c;
                            }
                            escaped += "\"";
                            csv << escaped;
                        } else {
                            csv << value;
                        }
                    }
                }
                csv << "\n";
            }
        }
    }
    
    return csv.str();
}

std::string FileWriter::jsonToXml(const std::string& jsonData) {
    // Basic JSON to XML conversion
    nlohmann::json data = nlohmann::json::parse(jsonData);
    std::stringstream xml;
    
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml << "<root>\n";
    
    std::function<void(const nlohmann::json&, int)> jsonToXmlRecursive = 
        [&](const nlohmann::json& obj, int indent) {
            std::string indentStr(indent * 2, ' ');
            
            if (obj.is_object()) {
                for (auto it = obj.begin(); it != obj.end(); ++it) {
                    xml << indentStr << "<" << it.key() << ">";
                    if (it->is_object() || it->is_array()) {
                        xml << "\n";
                        jsonToXmlRecursive(*it, indent + 1);
                        xml << indentStr;
                    } else {
                        xml << (it->is_string() ? it->get<std::string>() : it->dump());
                    }
                    xml << "</" << it.key() << ">\n";
                }
            } else if (obj.is_array()) {
                for (const auto& item : obj) {
                    xml << indentStr << "<item>";
                    if (item.is_object() || item.is_array()) {
                        xml << "\n";
                        jsonToXmlRecursive(item, indent + 1);
                        xml << indentStr;
                    } else {
                        xml << (item.is_string() ? item.get<std::string>() : item.dump());
                    }
                    xml << "</item>\n";
                }
            }
        };
    
    jsonToXmlRecursive(data, 1);
    xml << "</root>";
    
    return xml.str();
}

FileWriter::WriterStats FileWriter::getStatistics() const {
    return stats_;
}

void FileWriter::resetStatistics() {
    stats_.total_files_written = 0;
    stats_.total_bytes_written = 0;
    stats_.total_records_written = 0;
    stats_.total_processing_time = 0.0;
    stats_.format_distribution.clear();
}

bool FileWriter::fileExists(const std::string& filepath) {
    return std::filesystem::exists(filepath);
}

size_t FileWriter::getFileSize(const std::string& filepath) {
    try {
        return std::filesystem::file_size(filepath);
    } catch (const std::exception&) {
        return 0;
    }
}

} // namespace etl
