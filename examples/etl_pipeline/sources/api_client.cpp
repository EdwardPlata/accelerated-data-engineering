#include "api_client.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <functional>
#include <regex>

namespace etl {

ApiClient::ApiClient() 
    : curl_handle_(nullptr), default_headers_(nullptr), 
      timeout_(30), user_agent_("ETL-Pipeline-API-Client/1.0"),
      rate_limit_(10), delay_ms_(100), max_retries_(3), retry_delay_ms_(1000) {
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle_ = curl_easy_init();
    
    if (curl_handle_) {
        curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_handle_, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl_handle_, CURLOPT_USERAGENT, user_agent_.c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, timeout_);
        curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYHOST, 0L);
    }
}

ApiClient::~ApiClient() {
    if (default_headers_) {
        curl_slist_free_all(default_headers_);
    }
    if (curl_handle_) {
        curl_easy_cleanup(curl_handle_);
    }
    curl_global_cleanup();
}

void ApiClient::setBaseUrl(const std::string& baseUrl) {
    base_url_ = baseUrl;
    if (!base_url_.empty() && base_url_.back() == '/') {
        base_url_.pop_back();  // Remove trailing slash
    }
}

void ApiClient::addDefaultHeader(const std::string& key, const std::string& value) {
    std::string header = key + ": " + value;
    default_headers_ = curl_slist_append(default_headers_, header.c_str());
}

void ApiClient::setAuthentication(const std::string& type, const std::string& credentials) {
    auth_type_ = type;
    auth_credentials_ = credentials;
    
    if (type == "Bearer" || type == "bearer") {
        addDefaultHeader("Authorization", "Bearer " + credentials);
    } else if (type == "Basic" || type == "basic") {
        addDefaultHeader("Authorization", "Basic " + credentials);
    } else if (type == "ApiKey" || type == "apikey") {
        addDefaultHeader("X-API-Key", credentials);
    }
}

void ApiClient::setTimeout(long timeoutSeconds) {
    timeout_ = timeoutSeconds;
    if (curl_handle_) {
        curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, timeout_);
    }
}

void ApiClient::setUserAgent(const std::string& userAgent) {
    user_agent_ = userAgent;
    if (curl_handle_) {
        curl_easy_setopt(curl_handle_, CURLOPT_USERAGENT, user_agent_.c_str());
    }
}

void ApiClient::setRateLimit(int requestsPerSecond) {
    rate_limit_ = requestsPerSecond;
    if (requestsPerSecond > 0) {
        delay_ms_ = 1000 / requestsPerSecond;
    }
}

void ApiClient::setRetryPolicy(int maxRetries, int retryDelayMs) {
    max_retries_ = maxRetries;
    retry_delay_ms_ = retryDelayMs;
}

size_t ApiClient::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

size_t ApiClient::HeaderCallback(void* contents, size_t size, size_t nmemb, std::map<std::string, std::string>* userp) {
    size_t totalSize = size * nmemb;
    std::string header((char*)contents, totalSize);
    
    size_t colonPos = header.find(':');
    if (colonPos != std::string::npos) {
        std::string key = header.substr(0, colonPos);
        std::string value = header.substr(colonPos + 1);
        
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t\r\n") + 1);
        
        (*userp)[key] = value;
    }
    
    return totalSize;
}

ApiResponse ApiClient::get(const std::string& endpoint, const std::map<std::string, std::string>& params) {
    return request(HttpMethod::GET, endpoint, "", {}, params);
}

ApiResponse ApiClient::post(const std::string& endpoint, const std::string& body, const std::string& contentType) {
    std::map<std::string, std::string> headers = {{"Content-Type", contentType}};
    return request(HttpMethod::POST, endpoint, body, headers);
}

ApiResponse ApiClient::put(const std::string& endpoint, const std::string& body, const std::string& contentType) {
    std::map<std::string, std::string> headers = {{"Content-Type", contentType}};
    return request(HttpMethod::PUT, endpoint, body, headers);
}

ApiResponse ApiClient::delete_(const std::string& endpoint) {
    return request(HttpMethod::DELETE, endpoint);
}

ApiResponse ApiClient::patch(const std::string& endpoint, const std::string& body, const std::string& contentType) {
    std::map<std::string, std::string> headers = {{"Content-Type", contentType}};
    return request(HttpMethod::PATCH, endpoint, body, headers);
}

ApiResponse ApiClient::request(HttpMethod method, const std::string& endpoint, 
                              const std::string& body, 
                              const std::map<std::string, std::string>& headers,
                              const std::map<std::string, std::string>& params) {
    
    return retryRequest([=]() -> ApiResponse {
        respectRateLimit();
        
        std::string url = buildUrl(endpoint, params);
        setupRequest(url, headers);
        
        // Set HTTP method
        switch (method) {
            case HttpMethod::GET:
                curl_easy_setopt(curl_handle_, CURLOPT_HTTPGET, 1L);
                break;
            case HttpMethod::POST:
                curl_easy_setopt(curl_handle_, CURLOPT_POST, 1L);
                if (!body.empty()) {
                    curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, body.c_str());
                }
                break;
            case HttpMethod::PUT:
                curl_easy_setopt(curl_handle_, CURLOPT_CUSTOMREQUEST, "PUT");
                if (!body.empty()) {
                    curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, body.c_str());
                }
                break;
            case HttpMethod::DELETE:
                curl_easy_setopt(curl_handle_, CURLOPT_CUSTOMREQUEST, "DELETE");
                break;
            case HttpMethod::PATCH:
                curl_easy_setopt(curl_handle_, CURLOPT_CUSTOMREQUEST, "PATCH");
                if (!body.empty()) {
                    curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, body.c_str());
                }
                break;
        }
        
        return performRequest();
    });
}

std::string ApiClient::buildUrl(const std::string& endpoint, const std::map<std::string, std::string>& params) {
    std::string url = base_url_ + endpoint;
    
    if (!params.empty()) {
        url += "?";
        bool first = true;
        for (const auto& param : params) {
            if (!first) url += "&";
            url += urlEncode(param.first) + "=" + urlEncode(param.second);
            first = false;
        }
    }
    
    return url;
}

std::string ApiClient::urlEncode(const std::string& str) {
    char* encoded = curl_easy_escape(curl_handle_, str.c_str(), str.length());
    std::string result(encoded);
    curl_free(encoded);
    return result;
}

void ApiClient::setupRequest(const std::string& url, const std::map<std::string, std::string>& headers) {
    curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
    
    // Combine default headers with request-specific headers
    struct curl_slist* all_headers = nullptr;
    
    // Copy default headers
    struct curl_slist* current = default_headers_;
    while (current) {
        all_headers = curl_slist_append(all_headers, current->data);
        current = current->next;
    }
    
    // Add request-specific headers
    for (const auto& header : headers) {
        std::string headerStr = header.first + ": " + header.second;
        all_headers = curl_slist_append(all_headers, headerStr.c_str());
    }
    
    curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, all_headers);
}

ApiResponse ApiClient::performRequest() {
    ApiResponse response;
    std::string responseBody;
    std::map<std::string, std::string> responseHeaders;
    
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl_handle_, CURLOPT_HEADERDATA, &responseHeaders);
    
    auto start = std::chrono::high_resolution_clock::now();
    CURLcode res = curl_easy_perform(curl_handle_);
    auto end = std::chrono::high_resolution_clock::now();
    
    response.total_time = std::chrono::duration<double>(end - start).count();
    response.body = responseBody;
    response.headers = responseHeaders;
    
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &response.status_code);
        response.success = (response.status_code >= 200 && response.status_code < 300);
    } else {
        response.success = false;
        response.status_code = 0;
        response.error_message = curl_easy_strerror(res);
    }
    
    return response;
}

void ApiClient::respectRateLimit() {
    static auto lastRequest = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRequest);
    
    if (elapsed.count() < delay_ms_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms_ - elapsed.count()));
    }
    
    lastRequest = std::chrono::steady_clock::now();
}

ApiResponse ApiClient::retryRequest(const std::function<ApiResponse()>& requestFunc) {
    ApiResponse response;
    int attempts = 0;
    
    do {
        response = requestFunc();
        attempts++;
        
        if (response.success || attempts >= max_retries_) {
            break;
        }
        
        // Wait before retrying
        std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms_));
        
    } while (attempts < max_retries_);
    
    return response;
}

// API-specific implementations
ApiClient::WeatherData ApiClient::getWeatherData(const std::string& city, const std::string& apiKey) {
    WeatherData weather;
    
    // Example using OpenWeatherMap API
    std::map<std::string, std::string> params = {
        {"q", city},
        {"appid", apiKey},
        {"units", "metric"}
    };
    
    ApiResponse response = get("/weather", params);
    
    if (response.success) {
        // Parse JSON response (simplified - would use proper JSON parser in production)
        std::regex tempRegex("\"temp\":([0-9.]+)");
        std::regex descRegex("\"description\":\"([^\"]+)\"");
        std::regex humidityRegex("\"humidity\":([0-9.]+)");
        std::regex pressureRegex("\"pressure\":([0-9.]+)");
        
        std::smatch match;
        if (std::regex_search(response.body, match, tempRegex)) {
            weather.temperature = std::stod(match[1].str());
        }
        if (std::regex_search(response.body, match, descRegex)) {
            weather.description = match[1].str();
        }
        if (std::regex_search(response.body, match, humidityRegex)) {
            weather.humidity = std::stod(match[1].str());
        }
        if (std::regex_search(response.body, match, pressureRegex)) {
            weather.pressure = std::stod(match[1].str());
        }
        
        weather.location = city;
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        weather.timestamp = std::to_string(time_t);
    }
    
    return weather;
}

ApiClient::StockData ApiClient::getStockPrice(const std::string& symbol, const std::string& apiKey) {
    StockData stock;
    stock.symbol = symbol;
    
    // Example implementation - would vary based on actual API
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"apikey", apiKey}
    };
    
    ApiResponse response = get("/quote", params);
    
    if (response.success) {
        // Parse response (simplified)
        std::regex priceRegex("\"price\":([0-9.]+)");
        std::regex changeRegex("\"change\":([0-9.-]+)");
        
        std::smatch match;
        if (std::regex_search(response.body, match, priceRegex)) {
            stock.price = std::stod(match[1].str());
        }
        if (std::regex_search(response.body, match, changeRegex)) {
            stock.change = std::stod(match[1].str());
        }
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        stock.timestamp = std::to_string(time_t);
    }
    
    return stock;
}

std::vector<ApiClient::NewsItem> ApiClient::getNews(const std::string& category, const std::string& apiKey, int limit) {
    std::vector<NewsItem> news;
    
    std::map<std::string, std::string> params = {
        {"category", category},
        {"apiKey", apiKey},
        {"pageSize", std::to_string(limit)}
    };
    
    ApiResponse response = get("/top-headlines", params);
    
    if (response.success) {
        // Parse JSON array (simplified - would use proper JSON parser)
        // This is a basic implementation for demonstration
        std::regex articleRegex("\"title\":\"([^\"]+)\"[^}]*\"description\":\"([^\"]+)\"[^}]*\"url\":\"([^\"]+)\"");
        std::sregex_iterator iter(response.body.begin(), response.body.end(), articleRegex);
        std::sregex_iterator end;
        
        for (; iter != end && news.size() < static_cast<size_t>(limit); ++iter) {
            NewsItem item;
            item.title = (*iter)[1].str();
            item.description = (*iter)[2].str();
            item.url = (*iter)[3].str();
            item.category = category;
            
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            item.published_at = std::to_string(time_t);
            
            news.push_back(item);
        }
    }
    
    return news;
}

} // namespace etl
