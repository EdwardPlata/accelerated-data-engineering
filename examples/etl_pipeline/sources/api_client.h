#pragma once

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <functional>
#include <curl/curl.h>

#include <nlohmann/json.hpp>
#include <vector>
#include <memory>
#include <curl/curl.h>

namespace etl {

struct ApiResponse {
    std::string body;
    long status_code;
    std::map<std::string, std::string> headers;
    double total_time;
    std::string error_message;
    bool success;
};

enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH
};

class ApiClient {
public:
    ApiClient();
    ~ApiClient();

    // Configuration
    void setBaseUrl(const std::string& baseUrl);
    void addDefaultHeader(const std::string& key, const std::string& value);
    void setAuthentication(const std::string& type, const std::string& credentials);
    void setTimeout(long timeoutSeconds);
    void setUserAgent(const std::string& userAgent);

    // HTTP Methods
    ApiResponse get(const std::string& endpoint, const std::map<std::string, std::string>& params = {});
    ApiResponse post(const std::string& endpoint, const std::string& body, const std::string& contentType = "application/json");
    ApiResponse put(const std::string& endpoint, const std::string& body, const std::string& contentType = "application/json");
    ApiResponse delete_(const std::string& endpoint);
    ApiResponse patch(const std::string& endpoint, const std::string& body, const std::string& contentType = "application/json");

    // Generic request method
    ApiResponse request(HttpMethod method, const std::string& endpoint, 
                       const std::string& body = "", 
                       const std::map<std::string, std::string>& headers = {},
                       const std::map<std::string, std::string>& params = {});

    // Convenience methods for common APIs
    struct WeatherData {
        std::string location;
        double temperature;
        std::string description;
        double humidity;
        double pressure;
        std::string timestamp;
    };
    
    struct StockData {
        std::string symbol;
        double price;
        double change;
        double change_percent;
        std::string timestamp;
    };

    struct NewsItem {
        std::string title;
        std::string description;
        std::string url;
        std::string source;
        std::string published_at;
        std::string category;
    };

    // API-specific methods
    WeatherData getWeatherData(const std::string& city, const std::string& apiKey);
    StockData getStockPrice(const std::string& symbol, const std::string& apiKey);
    std::vector<NewsItem> getNews(const std::string& category, const std::string& apiKey, int limit = 10);

    // Batch operations
    std::vector<ApiResponse> batchRequests(const std::vector<std::pair<HttpMethod, std::string>>& requests);
    
    // Rate limiting
    void setRateLimit(int requestsPerSecond);
    
    // Retry logic
    void setRetryPolicy(int maxRetries, int retryDelayMs);

private:
    CURL* curl_handle_;
    struct curl_slist* default_headers_;
    std::string base_url_;
    std::string auth_type_;
    std::string auth_credentials_;
    long timeout_;
    std::string user_agent_;
    int rate_limit_;
    int delay_ms_;
    int max_retries_;
    int retry_delay_ms_;

    // Callback functions
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    static size_t HeaderCallback(void* contents, size_t size, size_t nmemb, std::map<std::string, std::string>* userp);

    // Helper methods
    std::string buildUrl(const std::string& endpoint, const std::map<std::string, std::string>& params);
    std::string urlEncode(const std::string& str);
    void setupRequest(const std::string& url, const std::map<std::string, std::string>& headers);
    ApiResponse performRequest();
    void respectRateLimit();
    ApiResponse retryRequest(const std::function<ApiResponse()>& requestFunc);

    // JSON utilities
    std::string createJsonPayload(const std::map<std::string, std::string>& data);
    std::map<std::string, std::string> parseJsonResponse(const std::string& json);
};

} // namespace etl
