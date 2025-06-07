#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <curl/curl.h>

namespace etl {

struct ScrapedData {
    std::string url;
    std::string title;
    std::string content;
    std::map<std::string, std::string> metadata;
    std::vector<std::string> links;
    std::vector<std::string> images;
    long response_code;
    std::string timestamp;
};

class WebScraper {
public:
    WebScraper();
    ~WebScraper();

    // Configuration
    void setUserAgent(const std::string& userAgent);
    void setTimeout(long timeoutSeconds);
    void setFollowRedirects(bool follow);
    void addHeader(const std::string& header);

    // Core scraping functionality
    ScrapedData scrapeUrl(const std::string& url);
    std::vector<ScrapedData> scrapeUrls(const std::vector<std::string>& urls);

    // HTML parsing utilities
    std::string extractTitle(const std::string& html);
    std::string extractText(const std::string& html);
    std::vector<std::string> extractLinks(const std::string& html, const std::string& baseUrl = "");
    std::vector<std::string> extractImages(const std::string& html, const std::string& baseUrl = "");
    std::string extractBySelector(const std::string& html, const std::string& selector);
    std::vector<std::string> extractAllBySelector(const std::string& html, const std::string& selector);

    // E-commerce specific extractors
    struct ProductData {
        std::string name;
        std::string price;
        std::string description;
        std::string image_url;
        std::string availability;
        std::map<std::string, std::string> attributes;
    };
    
    ProductData extractProductData(const std::string& html);
    std::vector<ProductData> extractProductListing(const std::string& html);

    // Rate limiting and politeness
    void setRateLimit(int requestsPerSecond);
    void setDelay(int milliseconds);

private:
    CURL* curl_handle_;
    struct curl_slist* headers_;
    std::string user_agent_;
    long timeout_;
    bool follow_redirects_;
    int rate_limit_;
    int delay_ms_;
    
    // Callback for writing received data
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    
    // HTTP request helper
    std::string performRequest(const std::string& url);
    
    // HTML utility functions
    std::string cleanText(const std::string& text);
    std::string resolveUrl(const std::string& url, const std::string& baseUrl);
    void respectRateLimit();
    
    // Simple HTML tag extraction (basic implementation)
    std::string extractBetweenTags(const std::string& html, const std::string& startTag, const std::string& endTag);
    std::vector<std::string> extractAllBetweenTags(const std::string& html, const std::string& startTag, const std::string& endTag);
    std::string extractAttributeValue(const std::string& tag, const std::string& attribute);
};

} // namespace etl
