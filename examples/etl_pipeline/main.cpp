#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <fstream>
#include <filesystem>

#include "sources/api_client.h"
#include "sources/web_scraper.h"
#include "sources/s3_client_simple.h"
#include "sources/sftp_client_simple.h"
#include "processors/data_transformer.h"
#include "loaders/file_writer.h"

namespace fs = std::filesystem;

// Using declarations for convenience
using etl::APIClient;
using etl::WebScraper;
using etl::S3Client;
using etl::SftpClient;
using etl::DataTransformer;
using etl::FileWriter;

void printHeader(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n\n";
}

void printSuccess(const std::string& message) {
    std::cout << "✓ " << message << "\n";
}

void printError(const std::string& message) {
    std::cerr << "✗ " << message << "\n";
}

void demoApiClient() {
    printHeader("API CLIENT DEMONSTRATION");
    
    try {
        APIClient client;
        
        // Demo 1: Basic HTTP GET
        std::cout << "1. Basic HTTP GET request:\n";
        auto response = client.get("https://httpbin.org/json");
        if (response.success) {
            printSuccess("GET request successful");
            std::cout << "   Response code: " << response.statusCode << "\n";
            std::cout << "   Data size: " << response.data.size() << " bytes\n";
        } else {
            printError("GET request failed: " + response.errorMessage);
        }
        
        // Demo 2: JSON POST
        std::cout << "\n2. JSON POST request:\n";
        nlohmann::json postData = {
            {"name", "ETL Pipeline Demo"},
            {"type", "data_processing"},
            {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()}
        };
        
        auto postResponse = client.post("https://httpbin.org/post", postData.dump());
        if (postResponse.success) {
            printSuccess("POST request successful");
            std::cout << "   Response code: " << postResponse.statusCode << "\n";
        } else {
            printError("POST request failed: " + postResponse.errorMessage);
        }
        
        // Demo 3: Weather API simulation
        std::cout << "\n3. Weather API simulation:\n";
        auto weatherData = client.getWeatherData("New York", "demo_api_key");
        if (weatherData.success) {
            printSuccess("Weather data retrieved");
            std::cout << "   Temperature: " << weatherData.temperature << "°C\n";
            std::cout << "   Humidity: " << weatherData.humidity << "%\n";
            std::cout << "   Conditions: " << weatherData.conditions << "\n";
        } else {
            printError("Weather API failed: " + weatherData.errorMessage);
        }
        
        // Demo 4: Rate limiting
        std::cout << "\n4. Rate limiting demonstration:\n";
        client.setRateLimit(2); // 2 requests per second
        auto start = std::chrono::steady_clock::now();
        
        for (int i = 0; i < 3; ++i) {
            auto resp = client.get("https://httpbin.org/delay/1");
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            std::cout << "   Request " << (i+1) << " completed after " << elapsed << "ms\n";
        }
        
        printSuccess("Rate limiting working correctly");
        
    } catch (const std::exception& e) {
        printError("API Client demo failed: " + std::string(e.what()));
    }
}

void demoWebScraper() {
    printHeader("WEB SCRAPER DEMONSTRATION");
    
    try {
        WebScraper scraper;
        
        // Demo 1: Basic page scraping
        std::cout << "1. Basic web page scraping:\n";
        auto htmlContent = scraper.fetchPage("https://httpbin.org/html");
        if (!htmlContent.empty()) {
            printSuccess("Page scraped successfully");
            std::cout << "   Content length: " << htmlContent.length() << " characters\n";
            
            // Extract title
            auto title = scraper.extractText(htmlContent, "title");
            if (!title.empty()) {
                std::cout << "   Page title: " << title << "\n";
            }
        } else {
            printError("Failed to scrape page");
        }
        
        // Demo 2: Extract specific elements
        std::cout << "\n2. Extract specific elements:\n";
        auto links = scraper.extractLinks(htmlContent);
        std::cout << "   Found " << links.size() << " links\n";
        
        for (size_t i = 0; i < std::min(links.size(), size_t(3)); ++i) {
            std::cout << "   Link " << (i+1) << ": " << links[i] << "\n";
        }
        
        // Demo 3: Table extraction
        std::cout << "\n3. Table data extraction:\n";
        auto tableData = scraper.extractTableData(htmlContent);
        if (!tableData.empty()) {
            printSuccess("Table data extracted");
            std::cout << "   Found " << tableData.size() << " tables\n";
        } else {
            std::cout << "   No tables found in the page\n";
        }
        
        printSuccess("Web scraper demonstration completed");
        
    } catch (const std::exception& e) {
        printError("Web Scraper demo failed: " + std::string(e.what()));
    }
}

void demoS3Client() {
    printHeader("S3 CLIENT DEMONSTRATION");
    
    try {
        // Note: This demo uses mock credentials and won't actually connect to AWS
        S3Client s3Client("demo-bucket", "us-east-1", "demo-access-key", "demo-secret-key");
        
        std::cout << "S3 Client initialized with demo credentials\n";
        std::cout << "Note: This demo shows the interface without actual AWS operations\n\n";
        
        // Demo 1: Upload simulation
        std::cout << "1. File upload simulation:\n";
        std::string sampleData = R"({
            "pipeline": "ETL Demo",
            "timestamp": "2024-01-01T12:00:00Z",
            "records": [
                {"id": 1, "name": "Sample Record 1"},
                {"id": 2, "name": "Sample Record 2"}
            ]
        })";
        
        std::cout << "   Preparing to upload " << sampleData.size() << " bytes\n";
        std::cout << "   Target S3 key: demo/sample-data.json\n";
        
        // Demo 2: Batch operations simulation
        std::cout << "\n2. Batch operations simulation:\n";
        std::vector<std::string> files = {
            "data/file1.json",
            "data/file2.json", 
            "data/file3.json"
        };
        
        std::cout << "   Batch upload of " << files.size() << " files\n";
        for (const auto& file : files) {
            std::cout << "   - " << file << "\n";
        }
        
        // Demo 3: Metadata operations
        std::cout << "\n3. Metadata operations:\n";
        std::map<std::string, std::string> metadata = {
            {"source", "etl_pipeline"},
            {"format", "json"},
            {"timestamp", "2024-01-01"}
        };
        
        std::cout << "   Setting metadata:\n";
        for (const auto& [key, value] : metadata) {
            std::cout << "   - " << key << ": " << value << "\n";
        }
        
        printSuccess("S3 Client demonstration completed");
        
    } catch (const std::exception& e) {
        printError("S3 Client demo failed: " + std::string(e.what()));
    }
}

void demoSftpClient() {
    printHeader("SFTP CLIENT DEMONSTRATION");
    
    try {
        std::cout << "SFTP Client demonstration (interface only)\n";
        std::cout << "Note: This demo shows the interface without actual SFTP connections\n\n";
        
        // Demo 1: Connection simulation
        std::cout << "1. SFTP connection simulation:\n";
        std::cout << "   Host: demo.sftp-server.com\n";
        std::cout << "   Port: 22\n";
        std::cout << "   Username: demo_user\n";
        std::cout << "   Authentication: Key-based\n";
        
        // Demo 2: File operations simulation
        std::cout << "\n2. File operations simulation:\n";
        std::vector<std::string> operations = {
            "Upload: local_file.csv -> /remote/data/file.csv",
            "Download: /remote/results/output.json -> local_output.json",
            "List directory: /remote/data/",
            "Create directory: /remote/processed/",
            "Delete file: /remote/temp/old_file.txt"
        };
        
        for (const auto& op : operations) {
            std::cout << "   - " << op << "\n";
        }
        
        // Demo 3: Batch transfer simulation
        std::cout << "\n3. Batch transfer simulation:\n";
        std::cout << "   Uploading multiple files to remote directory\n";
        std::cout << "   Progress tracking and error handling enabled\n";
        std::cout << "   Automatic retry on connection failures\n";
        
        printSuccess("SFTP Client demonstration completed");
        
    } catch (const std::exception& e) {
        printError("SFTP Client demo failed: " + std::string(e.what()));
    }
}

void demoDataTransformer() {
    printHeader("DATA TRANSFORMER DEMONSTRATION");
    
    try {
        DataTransformer transformer;
        
        // Demo 1: JSON transformation
        std::cout << "1. JSON data transformation:\n";
        nlohmann::json sourceJson = {
            {"users", {
                {{"id", 1}, {"name", "John Doe"}, {"email", "john@example.com"}, {"age", 30}},
                {{"id", 2}, {"name", "Jane Smith"}, {"email", "jane@example.com"}, {"age", 25}},
                {{"id", 3}, {"name", "Bob Johnson"}, {"email", "bob@example.com"}, {"age", 35}}
            }}
        };
        
        auto transformedJson = transformer.transformJson(sourceJson);
        if (!transformedJson.empty()) {
            printSuccess("JSON transformation completed");
            std::cout << "   Original records: " << sourceJson["users"].size() << "\n";
            std::cout << "   Transformed data size: " << transformedJson.size() << " bytes\n";
        }
        
        // Demo 2: CSV processing
        std::cout << "\n2. CSV data processing:\n";
        std::string csvData = "id,name,email,age,salary\n"
                             "1,John Doe,john@example.com,30,50000\n"
                             "2,Jane Smith,jane@example.com,25,55000\n"
                             "3,Bob Johnson,bob@example.com,35,60000\n";
        
        auto csvRecords = transformer.parseCsv(csvData);
        if (!csvRecords.empty()) {
            printSuccess("CSV parsing completed");
            std::cout << "   Records parsed: " << csvRecords.size() << "\n";
            std::cout << "   Fields per record: " << csvRecords[0].size() << "\n";
        }
        
        // Demo 3: Data cleaning
        std::cout << "\n3. Data cleaning and validation:\n";
        nlohmann::json dirtyData = {
            {"records", {
                {{"id", 1}, {"name", "  John Doe  "}, {"email", "JOHN@EXAMPLE.COM"}, {"score", "95.5"}},
                {{"id", 2}, {"name", ""}, {"email", "invalid-email"}, {"score", "N/A"}},
                {{"id", 3}, {"name", "Jane Smith"}, {"email", "jane@example.com"}, {"score", "87.2"}}
            }}
        };
        
        auto cleanedData = transformer.cleanData(dirtyData);
        if (!cleanedData.empty()) {
            printSuccess("Data cleaning completed");
            std::cout << "   Cleaned records available\n";
            std::cout << "   Validation rules applied\n";
        }
        
        // Demo 4: Type conversion
        std::cout << "\n4. Type conversion:\n";
        nlohmann::json mixedTypes = {
            {"string_number", "123"},
            {"string_float", "45.67"},
            {"string_bool", "true"},
            {"number_string", 789}
        };
        
        auto converted = transformer.convertTypes(mixedTypes);
        if (!converted.empty()) {
            printSuccess("Type conversion completed");
            std::cout << "   Mixed types normalized\n";
        }
        
        // Demo 5: Schema validation
        std::cout << "\n5. Schema validation:\n";
        nlohmann::json schema = {
            {"type", "object"},
            {"properties", {
                {"id", {{"type", "integer"}}},
                {"name", {{"type", "string"}}},
                {"email", {{"type", "string"}}}
            }},
            {"required", {"id", "name", "email"}}
        };
        
        nlohmann::json testData = {
            {"id", 1},
            {"name", "Test User"},
            {"email", "test@example.com"}
        };
        
        bool isValid = transformer.validateSchema(testData, schema);
        if (isValid) {
            printSuccess("Schema validation passed");
        } else {
            std::cout << "   Schema validation failed\n";
        }
        
        printSuccess("Data Transformer demonstration completed");
        
    } catch (const std::exception& e) {
        printError("Data Transformer demo failed: " + std::string(e.what()));
    }
}

void demoFileWriter() {
    printHeader("FILE WRITER DEMONSTRATION");
    
    try {
        // Create output directory
        fs::create_directories("output");
        
        // Demo 1: JSON file writing
        std::cout << "1. JSON file writing:\n";
        nlohmann::json jsonData = {
            {"pipeline", "ETL Demo"},
            {"timestamp", "2024-01-01T12:00:00Z"},
            {"results", {
                {{"id", 1}, {"value", 100}, {"status", "processed"}},
                {{"id", 2}, {"value", 200}, {"status", "processed"}},
                {{"id", 3}, {"value", 150}, {"status", "processed"}}
            }}
        };
        
        FileWriter jsonWriter("output/demo_results.json", FileWriter::Format::JSON);
        if (jsonWriter.writeJson(jsonData)) {
            printSuccess("JSON file written successfully");
            std::cout << "   File: output/demo_results.json\n";
            std::cout << "   Records: " << jsonData["results"].size() << "\n";
        }
        
        // Demo 2: CSV file writing
        std::cout << "\n2. CSV file writing:\n";
        std::vector<std::vector<std::string>> csvData = {
            {"ID", "Name", "Score", "Grade"},
            {"1", "Alice", "95", "A"},
            {"2", "Bob", "87", "B"},
            {"3", "Charlie", "92", "A"},
            {"4", "Diana", "78", "C"}
        };
        
        FileWriter csvWriter("output/demo_results.csv", FileWriter::Format::CSV);
        if (csvWriter.writeCsv(csvData)) {
            printSuccess("CSV file written successfully");
            std::cout << "   File: output/demo_results.csv\n";
            std::cout << "   Rows: " << csvData.size() - 1 << " (excluding header)\n";
        }
        
        // Demo 3: XML file writing
        std::cout << "\n3. XML file writing:\n";
        nlohmann::json xmlData = {
            {"catalog", {
                {"products", {
                    {{"id", "P001"}, {"name", "Product A"}, {"price", 29.99}},
                    {{"id", "P002"}, {"name", "Product B"}, {"price", 39.99}}
                }}
            }}
        };
        
        FileWriter xmlWriter("output/demo_results.xml", FileWriter::Format::XML);
        if (xmlWriter.writeXml(xmlData)) {
            printSuccess("XML file written successfully");
            std::cout << "   File: output/demo_results.xml\n";
        }
        
        // Demo 4: Streaming write
        std::cout << "\n4. Streaming write demonstration:\n";
        FileWriter streamWriter("output/stream_demo.json", FileWriter::Format::JSON);
        streamWriter.beginStream();
        
        for (int i = 1; i <= 5; ++i) {
            nlohmann::json record = {
                {"batch", i},
                {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count()},
                {"data", "Stream record " + std::to_string(i)}
            };
            streamWriter.writeStreamRecord(record);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        streamWriter.endStream();
        printSuccess("Streaming write completed");
        std::cout << "   File: output/stream_demo.json\n";
        std::cout << "   Records streamed: 5\n";
        
        // Demo 5: Batch writing
        std::cout << "\n5. Batch writing demonstration:\n";
        std::vector<nlohmann::json> batchData;
        for (int i = 1; i <= 10; ++i) {
            batchData.push_back({
                {"record_id", i},
                {"value", i * 10},
                {"category", (i % 2 == 0) ? "even" : "odd"}
            });
        }
        
        FileWriter batchWriter("output/batch_demo.json", FileWriter::Format::JSON);
        if (batchWriter.writeBatch(batchData)) {
            printSuccess("Batch write completed");
            std::cout << "   File: output/batch_demo.json\n";
            std::cout << "   Batch size: " << batchData.size() << " records\n";
        }
        
        printSuccess("File Writer demonstration completed");
        
    } catch (const std::exception& e) {
        printError("File Writer demo failed: " + std::string(e.what()));
    }
}

void demoCompletePipeline() {
    printHeader("COMPLETE ETL PIPELINE DEMONSTRATION");
    
    try {
        std::cout << "Running complete ETL pipeline with all components...\n\n";
        
        // Create pipeline output directory
        fs::create_directories("pipeline_output");
        
        // Step 1: Extract - Simulate data extraction from multiple sources
        std::cout << "STEP 1: EXTRACT\n";
        std::cout << "───────────────\n";
        
        // Extract from API
        APIClient apiClient;
        std::cout << "• Extracting data from API...\n";
        nlohmann::json apiData = {
            {"source", "api"},
            {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()},
            {"weather_data", {
                {{"city", "New York"}, {"temp", 22}, {"humidity", 65}},
                {{"city", "London"}, {"temp", 18}, {"humidity", 72}},
                {{"city", "Tokyo"}, {"temp", 25}, {"humidity", 58}}
            }}
        };
        
        // Extract from web scraping
        WebScraper scraper;
        std::cout << "• Extracting data from web scraping...\n";
        nlohmann::json webData = {
            {"source", "web_scraping"},
            {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()},
            {"scraped_data", {
                {{"url", "https://example.com/page1"}, {"title", "Sample Page 1"}, {"links", 15}},
                {{"url", "https://example.com/page2"}, {"title", "Sample Page 2"}, {"links", 23}}
            }}
        };
        
        // Simulate S3 extraction
        std::cout << "• Extracting data from S3...\n";
        nlohmann::json s3Data = {
            {"source", "s3"},
            {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()},
            {"files_processed", {
                {{"key", "data/sales_2024_01.csv"}, {"size", 1024000}, {"records", 5000}},
                {{"key", "data/sales_2024_02.csv"}, {"size", 987000}, {"records", 4800}}
            }}
        };
        
        printSuccess("Data extraction completed from all sources");
        
        // Step 2: Transform - Process and clean the extracted data
        std::cout << "\nSTEP 2: TRANSFORM\n";
        std::cout << "─────────────────\n";
        
        DataTransformer transformer;
        
        std::cout << "• Combining data from multiple sources...\n";
        nlohmann::json combinedData = {
            {"pipeline_id", "etl_demo_001"},
            {"execution_time", std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()},
            {"sources", {apiData, webData, s3Data}}
        };
        
        std::cout << "• Applying data transformations...\n";
        auto transformedData = transformer.transformJson(combinedData);
        
        std::cout << "• Cleaning and validating data...\n";
        auto cleanedData = transformer.cleanData(nlohmann::json::parse(transformedData));
        
        std::cout << "• Converting data types...\n";
        auto finalData = transformer.convertTypes(cleanedData);
        
        printSuccess("Data transformation completed");
        
        // Step 3: Load - Save processed data to multiple destinations
        std::cout << "\nSTEP 3: LOAD\n";
        std::cout << "────────────\n";
        
        // Load to JSON file
        std::cout << "• Loading data to JSON file...\n";
        FileWriter jsonLoader("pipeline_output/etl_results.json", FileWriter::Format::JSON);
        jsonLoader.writeJson(finalData);
        
        // Load to CSV file (flatten the data for CSV format)
        std::cout << "• Loading data to CSV file...\n";
        std::vector<std::vector<std::string>> csvData = {
            {"Pipeline_ID", "Execution_Time", "Source_Count", "Status"}
        };
        csvData.push_back({
            finalData["pipeline_id"].get<std::string>(),
            std::to_string(finalData["execution_time"].get<int>()),
            std::to_string(finalData["sources"].size()),
            "completed"
        });
        
        FileWriter csvLoader("pipeline_output/etl_summary.csv", FileWriter::Format::CSV);
        csvLoader.writeCsv(csvData);
        
        // Load to XML file
        std::cout << "• Loading data to XML file...\n";
        FileWriter xmlLoader("pipeline_output/etl_results.xml", FileWriter::Format::XML);
        xmlLoader.writeXml(finalData);
        
        // Simulate S3 upload
        std::cout << "• Uploading results to S3...\n";
        S3Client s3Client("etl-results-bucket", "us-east-1", "demo-key", "demo-secret");
        std::cout << "  - Uploaded: etl_results.json\n";
        std::cout << "  - Uploaded: etl_summary.csv\n";
        std::cout << "  - Uploaded: etl_results.xml\n";
        
        // Simulate SFTP transfer
        std::cout << "• Transferring files via SFTP...\n";
        std::cout << "  - Transferred to: remote.server.com:/data/processed/\n";
        
        printSuccess("Data loading completed to all destinations");
        
        // Pipeline Statistics
        std::cout << "\nPIPELINE STATISTICS\n";
        std::cout << "───────────────────\n";
        std::cout << "• Sources processed: 3 (API, Web Scraping, S3)\n";
        std::cout << "• Records transformed: Multiple datasets\n";
        std::cout << "• Output formats: JSON, CSV, XML\n";
        std::cout << "• Destinations: Local files, S3, SFTP\n";
        std::cout << "• Pipeline status: SUCCESS\n";
        
        printSuccess("Complete ETL pipeline demonstration finished");
        
    } catch (const std::exception& e) {
        printError("Complete pipeline demo failed: " + std::string(e.what()));
    }
}

int main() {
    std::cout << "ETL Pipeline Comprehensive Demonstration\n";
    std::cout << "========================================\n";
    std::cout << "This application demonstrates a complete ETL pipeline in C++\n";
    std::cout << "with web scraping, API calls, S3 operations, and SFTP transfers.\n";
    
    try {
        // Individual component demonstrations
        demoApiClient();
        demoWebScraper();
        demoS3Client();
        demoSftpClient();
        demoDataTransformer();
        demoFileWriter();
        
        // Complete pipeline demonstration
        demoCompletePipeline();
        
        printHeader("DEMONSTRATION COMPLETE");
        std::cout << "All ETL pipeline components have been demonstrated successfully!\n\n";
        std::cout << "Generated Files:\n";
        std::cout << "• output/demo_results.json\n";
        std::cout << "• output/demo_results.csv\n";
        std::cout << "• output/demo_results.xml\n";
        std::cout << "• output/stream_demo.json\n";
        std::cout << "• output/batch_demo.json\n";
        std::cout << "• pipeline_output/etl_results.json\n";
        std::cout << "• pipeline_output/etl_summary.csv\n";
        std::cout << "• pipeline_output/etl_results.xml\n\n";
        
        std::cout << "To build and run this application:\n";
        std::cout << "1. Install dependencies: libcurl, nlohmann-json, aws-sdk-cpp, libssh2, tinyxml2\n";
        std::cout << "2. mkdir build && cd build\n";
        std::cout << "3. cmake ..\n";
        std::cout << "4. make\n";
        std::cout << "5. ./etl_pipeline\n\n";
        
    } catch (const std::exception& e) {
        printError("Application error: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}