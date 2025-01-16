#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <zlib.h>
#include <stdexcept>

class CompressionHandler {
private:
    static constexpr size_t CHUNK_SIZE = 16384; // 16KB chunks for streaming
    static constexpr int COMPRESSION_LEVEL = Z_BEST_COMPRESSION;
    
    // Helper function to read file into chunks
    static std::vector<char> readFileInChunks(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Cannot open input file: " + filename);
        }
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            throw std::runtime_error("Error reading file: " + filename);
        }
        
        return buffer;
    }

public:
    static void compressFile(const std::string& inputFile, const std::string& outputFile) {
        try {
            // Read input file in chunks
            std::vector<char> inputData = readFileInChunks(inputFile);
            
            // Calculate maximum compressed size
            uLongf maxCompressedSize = compressBound(inputData.size());
            std::vector<Bytef> compressedData(maxCompressedSize);
            
            // Compress with best compression level
            z_stream zs;
            zs.zalloc = Z_NULL;
            zs.zfree = Z_NULL;
            zs.opaque = Z_NULL;
            
            if (deflateInit(&zs, COMPRESSION_LEVEL) != Z_OK) {
                throw std::runtime_error("deflateInit failed");
            }
            
            // Set up compression streams
            zs.avail_in = static_cast<uInt>(inputData.size());
            zs.next_in = reinterpret_cast<Bytef*>(inputData.data());
            zs.avail_out = static_cast<uInt>(maxCompressedSize);
            zs.next_out = compressedData.data();
            
            // Perform compression
            if (deflate(&zs, Z_FINISH) != Z_STREAM_END) {
                deflateEnd(&zs);
                throw std::runtime_error("deflate failed");
            }
            
            uLongf compressedSize = zs.total_out;
            deflateEnd(&zs);
            
            // Write compressed data
            std::ofstream output(outputFile, std::ios::binary);
            if (!output) {
                throw std::runtime_error("Cannot open output file: " + outputFile);
            }
            
            // Write original size first (needed for decompression)
            uint64_t originalSize = inputData.size();
            output.write(reinterpret_cast<const char*>(&originalSize), sizeof(originalSize));
            output.write(reinterpret_cast<const char*>(compressedData.data()), compressedSize);
            
            std::cout << "Compression successful!\n"
                      << "Original size: " << inputData.size() << " bytes\n"
                      << "Compressed size: " << compressedSize << " bytes\n"
                      << "Compression ratio: " << (float)compressedSize / inputData.size() * 100 << "%\n";
            
        } catch (const std::exception& e) {
            throw std::runtime_error("Compression error: " + std::string(e.what()));
        }
    }
    
    static void decompressFile(const std::string& inputFile, const std::string& outputFile) {
        try {
            // Read compressed file
            std::vector<char> compressedData = readFileInChunks(inputFile);
            
            // Read original size
            uint64_t originalSize;
            std::memcpy(&originalSize, compressedData.data(), sizeof(originalSize));
            
            // Prepare decompression
            std::vector<char> decompressedData(originalSize);
            
            z_stream zs;
            zs.zalloc = Z_NULL;
            zs.zfree = Z_NULL;
            zs.opaque = Z_NULL;
            zs.avail_in = static_cast<uInt>(compressedData.size() - sizeof(originalSize));
            zs.next_in = reinterpret_cast<Bytef*>(compressedData.data() + sizeof(originalSize));
            zs.avail_out = static_cast<uInt>(originalSize);
            zs.next_out = reinterpret_cast<Bytef*>(decompressedData.data());
            
            if (inflateInit(&zs) != Z_OK) {
                throw std::runtime_error("inflateInit failed");
            }
            
            // Perform decompression
            if (inflate(&zs, Z_FINISH) != Z_STREAM_END) {
                inflateEnd(&zs);
                throw std::runtime_error("inflate failed");
            }
            
            inflateEnd(&zs);
            
            // Write decompressed data
            std::ofstream output(outputFile, std::ios::binary);
            if (!output) {
                throw std::runtime_error("Cannot open output file: " + outputFile);
            }
            
            output.write(decompressedData.data(), originalSize);
            
            std::cout << "Decompression successful!\n"
                      << "Decompressed size: " << originalSize << " bytes\n";
            
        } catch (const std::exception& e) {
            throw std::runtime_error("Decompression error: " + std::string(e.what()));
        }
    }
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 4) {
            std::cout << "Usage: " << argv[0] << " [c/d] input_file output_file\n"
                      << "  c - compress\n"
                      << "  d - decompress\n";
            return 1;
        }
        
        std::string operation = argv[1];
        std::string inputFile = argv[2];
        std::string outputFile = argv[3];
        
        if (operation == "c") {
            CompressionHandler::compressFile(inputFile, outputFile);
        } else if (operation == "d") {
            CompressionHandler::decompressFile(inputFile, outputFile);
        } else {
            std::cerr << "Invalid operation. Use 'c' for compression or 'd' for decompression.\n";
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}