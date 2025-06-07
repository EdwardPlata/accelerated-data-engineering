#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>

namespace etl {

struct TransformationResult {
    bool success;
    std::string error_message;
    std::string output_data;
    std::map<std::string, std::string> metadata;
    size_t input_size;
    size_t output_size;
    double processing_time;
};

struct DataSchema {
    std::map<std::string, std::string> field_types; // field_name -> type (string, int, float, date, etc.)
    std::vector<std::string> required_fields;
    std::map<std::string, std::string> field_mappings; // old_name -> new_name
    std::map<std::string, std::function<std::string(const std::string&)>> field_transformers;
};

class DataTransformer {
public:
    DataTransformer();
    ~DataTransformer();

    // JSON Processing
    TransformationResult processJson(const std::string& jsonData, const DataSchema& schema);
    TransformationResult transformJsonStructure(const std::string& jsonData, 
                                               const std::map<std::string, std::string>& fieldMappings);
    TransformationResult flattenJson(const std::string& jsonData, const std::string& separator = ".");
    TransformationResult filterJsonFields(const std::string& jsonData, 
                                         const std::vector<std::string>& fieldsToKeep);
    
    // CSV Processing
    TransformationResult processCsv(const std::string& csvData, const DataSchema& schema);
    TransformationResult csvToJson(const std::string& csvData, bool hasHeader = true);
    TransformationResult jsonToCsv(const std::string& jsonData, 
                                  const std::vector<std::string>& columnOrder = {});
    TransformationResult transformCsvColumns(const std::string& csvData, 
                                            const std::map<std::string, std::string>& columnMappings);
    
    // Data Cleaning
    TransformationResult cleanData(const std::string& data, const std::string& format);
    TransformationResult removeNullValues(const std::string& jsonData);
    TransformationResult standardizeValues(const std::string& data, 
                                         const std::map<std::string, std::map<std::string, std::string>>& mappings);
    TransformationResult deduplicateRecords(const std::string& jsonArrayData, 
                                           const std::vector<std::string>& keyFields);
    
    // Data Validation
    struct ValidationResult {
        bool is_valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
        int valid_records;
        int invalid_records;
    };
    
    ValidationResult validateData(const std::string& data, const DataSchema& schema);
    ValidationResult validateJson(const std::string& jsonData, const DataSchema& schema);
    ValidationResult validateCsv(const std::string& csvData, const DataSchema& schema);
    
    // Data Type Conversions
    TransformationResult convertDataTypes(const std::string& jsonData, 
                                         const std::map<std::string, std::string>& typeConversions);
    std::string convertStringToType(const std::string& value, const std::string& targetType);
    bool isValidType(const std::string& value, const std::string& type);
    
    // Aggregation and Grouping
    TransformationResult aggregateData(const std::string& jsonArrayData, 
                                      const std::vector<std::string>& groupByFields,
                                      const std::map<std::string, std::string>& aggregations);
    
    // Date/Time Processing
    TransformationResult standardizeDates(const std::string& jsonData, 
                                         const std::vector<std::string>& dateFields,
                                         const std::string& inputFormat = "",
                                         const std::string& outputFormat = "ISO8601");
    
    // Text Processing
    TransformationResult normalizeText(const std::string& jsonData, 
                                      const std::vector<std::string>& textFields);
    TransformationResult extractKeywords(const std::string& text, int maxKeywords = 10);
    
    // Statistical Transformations
    struct DataStats {
        double mean;
        double median;
        double std_dev;
        double min_value;
        double max_value;
        int count;
    };
    
    DataStats calculateStats(const std::vector<double>& values);
    TransformationResult normalizeNumericFields(const std::string& jsonData, 
                                               const std::vector<std::string>& numericFields,
                                               const std::string& method = "z-score"); // z-score, min-max
    
    // Utility Methods
    void setDefaultDateFormat(const std::string& format);
    void addCustomTransformer(const std::string& name, 
                             const std::function<std::string(const std::string&)>& transformer);
    void setErrorTolerance(bool continueOnError);
    
    // ETL Pipeline Integration
    TransformationResult processDataPipeline(const std::string& inputData, 
                                            const std::vector<std::string>& transformationSteps);
    
private:
    std::string default_date_format_;
    std::map<std::string, std::function<std::string(const std::string&)>> custom_transformers_;
    bool continue_on_error_;
    
    // Helper methods
    nlohmann::json parseJsonSafely(const std::string& jsonStr);
    std::vector<std::vector<std::string>> parseCsv(const std::string& csvData);
    std::string escapeCSVField(const std::string& field);
    std::vector<std::string> splitString(const std::string& str, char delimiter);
    std::string trimString(const std::string& str);
    bool isNumeric(const std::string& str);
    bool isDate(const std::string& str, const std::string& format = "");
    std::string formatDate(const std::string& dateStr, const std::string& inputFormat, 
                          const std::string& outputFormat);
    double stringToDouble(const std::string& str);
    std::string doubleToString(double value, int precision = 2);
};

} // namespace etl
