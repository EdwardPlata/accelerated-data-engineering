# ETL Pipeline Examples in C++

This project demonstrates various ETL (Extract, Transform, Load) pipeline implementations in C++ with real-world data sources including:

## Features

### Data Sources (Extract)
- **Web Scraping**: HTML parsing and data extraction from websites
- **REST API Calls**: HTTP client for consuming RESTful APIs
- **S3 Blob Storage**: AWS S3 integration for cloud storage
- **SFTP Servers**: Secure file transfer protocol for remote file access

### Data Processing (Transform)
- JSON parsing and manipulation
- CSV processing
- Data validation and cleaning
- Schema transformation

### Data Loading (Load)
- Local file system
- Database integration
- Cloud storage upload

## Dependencies

This project uses the following C++ libraries:
- **libcurl**: HTTP/HTTPS requests and SFTP operations
- **nlohmann/json**: JSON parsing and manipulation
- **aws-sdk-cpp**: AWS S3 operations
- **libssh2**: SFTP client operations
- **tinyxml2**: XML/HTML parsing

## Building

```bash
mkdir build && cd build
cmake ..
make
```

## Running Examples

```bash
# Run the main ETL pipeline
./etl_pipeline

# Run specific examples
./etl_pipeline --webscraping
./etl_pipeline --api-calls
./etl_pipeline --s3-operations
./etl_pipeline --sftp-operations
```

## Project Structure

```
etl_pipeline/
├── main.cpp                 # Main application entry point
├── sources/
│   ├── web_scraper.h/cpp    # Web scraping implementation
│   ├── api_client.h/cpp     # REST API client
│   ├── s3_client.h/cpp      # AWS S3 operations
│   └── sftp_client.h/cpp    # SFTP operations
├── processors/
│   ├── data_transformer.h/cpp # Data transformation utilities
│   └── validator.h/cpp      # Data validation
└── loaders/
    ├── file_writer.h/cpp    # File output operations
    └── db_writer.h/cpp      # Database operations
```

## Examples

### Web Scraping Example
Extracts product information from e-commerce websites, parses HTML content, and structures data for analysis.

### API Integration Example
Connects to public APIs (weather, stock prices, news), handles authentication, and processes JSON responses.

### S3 Operations Example
Demonstrates uploading/downloading files to/from AWS S3, handling large datasets, and managing bucket operations.

### SFTP Operations Example
Shows secure file transfer operations, directory listing, and batch file processing from remote servers.