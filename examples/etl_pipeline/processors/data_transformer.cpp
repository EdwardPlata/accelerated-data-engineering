#include "data_transformer.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <chrono>
#include <cmath>
#include <iomanip>

namespace etl {

DataTransformer::DataTransformer() 
    : default_date_format_("YYYY-MM-DD"), continue_on_error_(true) {}

DataTransformer::~DataTransformer() {}

TransformationResult DataTransformer::processJson(const std::string& jsonData, const DataSchema& schema) {
    TransformationResult result;
    result.input_size = jsonData.length();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        nlohmann::json data = parseJsonSafely(jsonData);
        
        if (data.is_null()) {
            result.success = false;
            result.error_message = "Invalid JSON format";
            return result;
        }
        
        // Apply field mappings
        if (!schema.field_mappings.empty()) {
            for (const auto& mapping : schema.field_mappings) {
                if (data.contains(mapping.first)) {
                    data[mapping.second] = data[mapping.first];
                    data.erase(mapping.first);
                }
            }
        }
        
        // Apply field transformers
        for (const auto& transformer : schema.field_transformers) {
            if (data.contains(transformer.first) && data[transformer.first].is_string()) {
                std::string value = data[transformer.first];
                data[transformer.first] = transformer.second(value);
            }
        }
        
        // Validate required fields
        for (const auto& field : schema.required_fields) {
            if (!data.contains(field)) {
                result.success = false;
                result.error_message = "Missing required field: " + field;
                return result;
            }
        }
        
        result.output_data = data.dump(4);
        result.success = true;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.processing_time = std::chrono::duration<double>(end - start).count();
    result.output_size = result.output_data.length();
    
    return result;
}

TransformationResult DataTransformer::csvToJson(const std::string& csvData, bool hasHeader) {
    TransformationResult result;
    result.input_size = csvData.length();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        auto rows = parseCsv(csvData);
        if (rows.empty()) {
            result.success = false;
            result.error_message = "Empty CSV data";
            return result;
        }
        
        nlohmann::json jsonArray = nlohmann::json::array();
        std::vector<std::string> headers;
        
        size_t startRow = 0;
        if (hasHeader && !rows.empty()) {
            headers = rows[0];
            startRow = 1;
        } else {
            // Generate default headers
            for (size_t i = 0; i < rows[0].size(); ++i) {
                headers.push_back("column_" + std::to_string(i));
            }
        }
        
        for (size_t i = startRow; i < rows.size(); ++i) {
            nlohmann::json record;
            for (size_t j = 0; j < headers.size() && j < rows[i].size(); ++j) {
                record[headers[j]] = rows[i][j];
            }
            jsonArray.push_back(record);
        }
        
        result.output_data = jsonArray.dump(4);
        result.success = true;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.processing_time = std::chrono::duration<double>(end - start).count();
    result.output_size = result.output_data.length();
    
    return result;
}

TransformationResult DataTransformer::jsonToCsv(const std::string& jsonData, 
                                               const std::vector<std::string>& columnOrder) {
    TransformationResult result;
    result.input_size = jsonData.length();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        nlohmann::json data = parseJsonSafely(jsonData);
        
        if (!data.is_array()) {
            result.success = false;
            result.error_message = "JSON data must be an array for CSV conversion";
            return result;
        }
        
        std::stringstream csvOutput;
        std::vector<std::string> headers;
        
        // Determine headers
        if (!columnOrder.empty()) {
            headers = columnOrder;
        } else if (!data.empty() && data[0].is_object()) {
            for (auto it = data[0].begin(); it != data[0].end(); ++it) {
                headers.push_back(it.key());
            }
        }
        
        // Write headers
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i > 0) csvOutput << ",";
            csvOutput << escapeCSVField(headers[i]);
        }
        csvOutput << "\n";
        
        // Write data rows
        for (const auto& record : data) {
            if (record.is_object()) {
                for (size_t i = 0; i < headers.size(); ++i) {
                    if (i > 0) csvOutput << ",";
                    
                    if (record.contains(headers[i])) {
                        std::string value = record[headers[i]].is_string() 
                            ? record[headers[i]].get<std::string>()
                            : record[headers[i]].dump();
                        csvOutput << escapeCSVField(value);
                    }
                }
                csvOutput << "\n";
            }
        }
        
        result.output_data = csvOutput.str();
        result.success = true;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.processing_time = std::chrono::duration<double>(end - start).count();
    result.output_size = result.output_data.length();
    
    return result;
}

TransformationResult DataTransformer::cleanData(const std::string& data, const std::string& format) {
    TransformationResult result;
    result.input_size = data.length();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        if (format == "json") {
            nlohmann::json jsonData = parseJsonSafely(data);
            
            // Remove null values, empty strings, and invalid data
            std::function<void(nlohmann::json&)> cleanJson = [&](nlohmann::json& obj) {
                if (obj.is_object()) {
                    auto it = obj.begin();
                    while (it != obj.end()) {
                        if (it->is_null() || 
                            (it->is_string() && it->get<std::string>().empty()) ||
                            (it->is_string() && trimString(it->get<std::string>()).empty())) {
                            it = obj.erase(it);
                        } else {
                            if (it->is_object() || it->is_array()) {
                                cleanJson(*it);
                            }
                            ++it;
                        }
                    }
                } else if (obj.is_array()) {
                    for (auto& element : obj) {
                        cleanJson(element);
                    }
                }
            };
            
            cleanJson(jsonData);
            result.output_data = jsonData.dump(4);
            
        } else if (format == "csv") {
            auto rows = parseCsv(data);
            std::stringstream cleanCsv;
            
            for (const auto& row : rows) {
                std::vector<std::string> cleanedRow;
                for (const auto& field : row) {
                    std::string cleanField = trimString(field);
                    if (!cleanField.empty() && cleanField != "NULL" && cleanField != "null") {
                        cleanedRow.push_back(cleanField);
                    } else {
                        cleanedRow.push_back(""); // Keep structure but empty value
                    }
                }
                
                for (size_t i = 0; i < cleanedRow.size(); ++i) {
                    if (i > 0) cleanCsv << ",";
                    cleanCsv << escapeCSVField(cleanedRow[i]);
                }
                cleanCsv << "\n";
            }
            
            result.output_data = cleanCsv.str();
        } else {
            result.success = false;
            result.error_message = "Unsupported format: " + format;
            return result;
        }
        
        result.success = true;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.processing_time = std::chrono::duration<double>(end - start).count();
    result.output_size = result.output_data.length();
    
    return result;
}

DataTransformer::ValidationResult DataTransformer::validateJson(const std::string& jsonData, 
                                                               const DataSchema& schema) {
    ValidationResult result;
    result.is_valid = true;
    result.valid_records = 0;
    result.invalid_records = 0;
    
    try {
        nlohmann::json data = parseJsonSafely(jsonData);
        
        if (data.is_null()) {
            result.is_valid = false;
            result.errors.push_back("Invalid JSON format");
            return result;
        }
        
        std::function<void(const nlohmann::json&, int)> validateRecord = 
            [&](const nlohmann::json& record, int recordIndex) {
                bool recordValid = true;
                
                // Check required fields
                for (const auto& field : schema.required_fields) {
                    if (!record.contains(field)) {
                        result.errors.push_back("Record " + std::to_string(recordIndex) + 
                                               ": Missing required field '" + field + "'");
                        recordValid = false;
                    }
                }
                
                // Check field types
                for (const auto& fieldType : schema.field_types) {
                    if (record.contains(fieldType.first)) {
                        const auto& value = record[fieldType.first];
                        std::string valueStr = value.is_string() ? value.get<std::string>() : value.dump();
                        
                        if (!isValidType(valueStr, fieldType.second)) {
                            result.errors.push_back("Record " + std::to_string(recordIndex) + 
                                                   ": Invalid type for field '" + fieldType.first + 
                                                   "', expected " + fieldType.second);
                            recordValid = false;
                        }
                    }
                }
                
                if (recordValid) {
                    result.valid_records++;
                } else {
                    result.invalid_records++;
                    result.is_valid = false;
                }
            };
        
        if (data.is_array()) {
            for (size_t i = 0; i < data.size(); ++i) {
                validateRecord(data[i], i);
            }
        } else {
            validateRecord(data, 0);
        }
        
    } catch (const std::exception& e) {
        result.is_valid = false;
        result.errors.push_back("Validation error: " + std::string(e.what()));
    }
    
    return result;
}

TransformationResult DataTransformer::convertDataTypes(const std::string& jsonData, 
                                                      const std::map<std::string, std::string>& typeConversions) {
    TransformationResult result;
    result.input_size = jsonData.length();
    
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        nlohmann::json data = parseJsonSafely(jsonData);
        
        std::function<void(nlohmann::json&)> convertTypes = [&](nlohmann::json& obj) {
            if (obj.is_object()) {
                for (auto& [key, value] : obj.items()) {
                    if (typeConversions.find(key) != typeConversions.end()) {
                        std::string targetType = typeConversions.at(key);
                        std::string currentValue = value.is_string() ? value.get<std::string>() : value.dump();
                        
                        try {
                            std::string convertedValue = convertStringToType(currentValue, targetType);
                            
                            if (targetType == "int") {
                                obj[key] = std::stoi(convertedValue);
                            } else if (targetType == "float" || targetType == "double") {
                                obj[key] = std::stod(convertedValue);
                            } else if (targetType == "bool") {
                                obj[key] = (convertedValue == "true" || convertedValue == "1");
                            } else {
                                obj[key] = convertedValue;
                            }
                        } catch (const std::exception& e) {
                            if (!continue_on_error_) {
                                throw;
                            }
                            result.metadata["conversion_errors"] += 
                                "Failed to convert " + key + ": " + e.what() + "; ";
                        }
                    }
                    
                    if (value.is_object() || value.is_array()) {
                        convertTypes(value);
                    }
                }
            } else if (obj.is_array()) {
                for (auto& element : obj) {
                    convertTypes(element);
                }
            }
        };
        
        convertTypes(data);
        result.output_data = data.dump(4);
        result.success = true;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    result.processing_time = std::chrono::duration<double>(end - start).count();
    result.output_size = result.output_data.length();
    
    return result;
}

// Helper method implementations

nlohmann::json DataTransformer::parseJsonSafely(const std::string& jsonStr) {
    try {
        return nlohmann::json::parse(jsonStr);
    } catch (const std::exception&) {
        return nlohmann::json();
    }
}

std::vector<std::vector<std::string>> DataTransformer::parseCsv(const std::string& csvData) {
    std::vector<std::vector<std::string>> rows;
    std::istringstream stream(csvData);
    std::string line;
    
    while (std::getline(stream, line)) {
        std::vector<std::string> row;
        std::stringstream lineStream(line);
        std::string cell;
        
        while (std::getline(lineStream, cell, ',')) {
            // Basic CSV parsing - in production, you'd want more robust parsing
            cell = trimString(cell);
            if (cell.front() == '"' && cell.back() == '"') {
                cell = cell.substr(1, cell.length() - 2);
            }
            row.push_back(cell);
        }
        
        if (!row.empty()) {
            rows.push_back(row);
        }
    }
    
    return rows;
}

std::string DataTransformer::escapeCSVField(const std::string& field) {
    if (field.find(',') != std::string::npos || 
        field.find('"') != std::string::npos || 
        field.find('\n') != std::string::npos) {
        std::string escaped = "\"";
        for (char c : field) {
            if (c == '"') {
                escaped += "\"\"";
            } else {
                escaped += c;
            }
        }
        escaped += "\"";
        return escaped;
    }
    return field;
}

std::string DataTransformer::trimString(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool DataTransformer::isNumeric(const std::string& str) {
    try {
        std::stod(str);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool DataTransformer::isValidType(const std::string& value, const std::string& type) {
    if (type == "string") return true;
    if (type == "int") {
        try { std::stoi(value); return true; } catch (...) { return false; }
    }
    if (type == "float" || type == "double") {
        return isNumeric(value);
    }
    if (type == "bool") {
        return (value == "true" || value == "false" || value == "0" || value == "1");
    }
    if (type == "date") {
        return isDate(value);
    }
    return false;
}

bool DataTransformer::isDate(const std::string& str, const std::string& format) {
    // Basic date validation - in production, you'd use a proper date library
    std::regex dateRegex(R"(\d{4}-\d{2}-\d{2})"); // YYYY-MM-DD format
    return std::regex_match(str, dateRegex);
}

std::string DataTransformer::convertStringToType(const std::string& value, const std::string& targetType) {
    if (targetType == "string") return value;
    if (targetType == "int") return std::to_string(std::stoi(value));
    if (targetType == "float" || targetType == "double") return std::to_string(std::stod(value));
    if (targetType == "bool") {
        if (value == "1" || value == "true" || value == "True" || value == "TRUE") return "true";
        return "false";
    }
    return value;
}

void DataTransformer::setErrorTolerance(bool continueOnError) {
    continue_on_error_ = continueOnError;
}

} // namespace etl
