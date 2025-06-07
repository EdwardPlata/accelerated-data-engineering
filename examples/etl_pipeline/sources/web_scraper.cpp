#include "web_scraper.h"
#include <iostream>
#include <regex>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cctype>

namespace etl {

WebScraper::WebScraper() 
    : curl_handle_(nullptr), headers_(nullptr), 
      user_agent_("ETL-Pipeline/1.0 (Educational Example)"),
      timeout_(30), follow_redirects_(true), rate_limit_(1), delay_ms_(1000) {
    
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle_ = curl_easy_init();
    
    if (curl_handle_) {
        curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl_handle_, CURLOPT_USERAGENT, user_agent_.c_str());
        curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, timeout_);
        curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, follow_redirects_ ? 1L : 0L);
        curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYHOST, 0L);
    }
}

WebScraper::~WebScraper() {
    if (headers_) {
        curl_slist_free_all(headers_);
    }
    if (curl_handle_) {
        curl_easy_cleanup(curl_handle_);
    }
    curl_global_cleanup();
}

void WebScraper::setUserAgent(const std::string& userAgent) {
    user_agent_ = userAgent;
    if (curl_handle_) {
        curl_easy_setopt(curl_handle_, CURLOPT_USERAGENT, user_agent_.c_str());
    }
}

void WebScraper::setTimeout(long timeoutSeconds) {
    timeout_ = timeoutSeconds;
    if (curl_handle_) {
        curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, timeout_);
    }
}

void WebScraper::setFollowRedirects(bool follow) {
    follow_redirects_ = follow;
    if (curl_handle_) {
        curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, follow ? 1L : 0L);
    }
}

void WebScraper::addHeader(const std::string& header) {
    headers_ = curl_slist_append(headers_, header.c_str());
    if (curl_handle_) {
        curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, headers_);
    }
}

void WebScraper::setRateLimit(int requestsPerSecond) {
    rate_limit_ = requestsPerSecond;
    if (requestsPerSecond > 0) {
        delay_ms_ = 1000 / requestsPerSecond;
    }
}

void WebScraper::setDelay(int milliseconds) {
    delay_ms_ = milliseconds;
}

size_t WebScraper::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

std::string WebScraper::performRequest(const std::string& url) {
    if (!curl_handle_) {
        throw std::runtime_error("CURL not initialized");
    }

    respectRateLimit();

    std::string response_data;
    curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &response_data);

    CURLcode res = curl_easy_perform(curl_handle_);
    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }

    return response_data;
}

ScrapedData WebScraper::scrapeUrl(const std::string& url) {
    ScrapedData data;
    data.url = url;
    
    try {
        std::string html = performRequest(url);
        
        // Get response code
        curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &data.response_code);
        
        // Extract data
        data.title = extractTitle(html);
        data.content = extractText(html);
        data.links = extractLinks(html, url);
        data.images = extractImages(html, url);
        
        // Add timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        data.timestamp = std::to_string(time_t);
        
    } catch (const std::exception& e) {
        std::cerr << "Error scraping " << url << ": " << e.what() << std::endl;
        data.response_code = 0;
    }
    
    return data;
}

std::vector<ScrapedData> WebScraper::scrapeUrls(const std::vector<std::string>& urls) {
    std::vector<ScrapedData> results;
    results.reserve(urls.size());
    
    for (const auto& url : urls) {
        results.push_back(scrapeUrl(url));
    }
    
    return results;
}

std::string WebScraper::extractTitle(const std::string& html) {
    std::regex titleRegex("<title[^>]*>([^<]+)</title>", std::regex_constants::icase);
    std::smatch match;
    
    if (std::regex_search(html, match, titleRegex)) {
        return cleanText(match[1].str());
    }
    return "";
}

std::string WebScraper::extractText(const std::string& html) {
    // Remove script and style tags
    std::regex scriptRegex("<script[^>]*>.*?</script>", std::regex_constants::icase);
    std::regex styleRegex("<style[^>]*>.*?</style>", std::regex_constants::icase);
    
    std::string cleanedHtml = std::regex_replace(html, scriptRegex, "");
    cleanedHtml = std::regex_replace(cleanedHtml, styleRegex, "");
    
    // Remove HTML tags
    std::regex tagRegex("<[^>]*>");
    std::string text = std::regex_replace(cleanedHtml, tagRegex, " ");
    
    return cleanText(text);
}

std::vector<std::string> WebScraper::extractLinks(const std::string& html, const std::string& baseUrl) {
    std::vector<std::string> links;
    std::regex linkRegex("<a[^>]+href\\s*=\\s*[\"']([^\"']+)[\"']", std::regex_constants::icase);
    std::sregex_iterator iter(html.begin(), html.end(), linkRegex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::string link = (*iter)[1].str();
        if (!baseUrl.empty()) {
            link = resolveUrl(link, baseUrl);
        }
        links.push_back(link);
    }
    
    return links;
}

std::vector<std::string> WebScraper::extractImages(const std::string& html, const std::string& baseUrl) {
    std::vector<std::string> images;
    std::regex imgRegex("<img[^>]+src\\s*=\\s*[\"']([^\"']+)[\"']", std::regex_constants::icase);
    std::sregex_iterator iter(html.begin(), html.end(), imgRegex);
    std::sregex_iterator end;
    
    for (; iter != end; ++iter) {
        std::string src = (*iter)[1].str();
        if (!baseUrl.empty()) {
            src = resolveUrl(src, baseUrl);
        }
        images.push_back(src);
    }
    
    return images;
}

WebScraper::ProductData WebScraper::extractProductData(const std::string& html) {
    ProductData product;
    
    // Extract product name (try common selectors)
    std::regex nameRegex("<h1[^>]*>([^<]+)</h1>|<h2[^>]*>([^<]+)</h2>", std::regex_constants::icase);
    std::smatch match;
    if (std::regex_search(html, match, nameRegex)) {
        product.name = cleanText(match[1].str().empty() ? match[2].str() : match[1].str());
    }
    
    // Extract price (basic patterns)
    std::regex priceRegex("\\$([0-9,]+\\.?[0-9]*)|([0-9,]+\\.?[0-9]*)\\s*USD", std::regex_constants::icase);
    if (std::regex_search(html, match, priceRegex)) {
        product.price = match[0].str();
    }
    
    // Extract description (look for meta description or first paragraph)
    std::regex descRegex("<meta[^>]+name\\s*=\\s*[\"']description[\"'][^>]+content\\s*=\\s*[\"']([^\"']+)[\"']", std::regex_constants::icase);
    if (std::regex_search(html, match, descRegex)) {
        product.description = cleanText(match[1].str());
    }
    
    return product;
}

std::string WebScraper::cleanText(const std::string& text) {
    std::string cleaned = text;
    
    // Replace HTML entities
    std::regex entityRegex("&([a-zA-Z]+|#[0-9]+);");
    cleaned = std::regex_replace(cleaned, entityRegex, " ");
    
    // Replace multiple whitespace with single space
    std::regex whitespaceRegex("\\s+");
    cleaned = std::regex_replace(cleaned, whitespaceRegex, " ");
    
    // Trim leading and trailing whitespace
    cleaned.erase(0, cleaned.find_first_not_of(" \t\n\r"));
    cleaned.erase(cleaned.find_last_not_of(" \t\n\r") + 1);
    
    return cleaned;
}

std::string WebScraper::resolveUrl(const std::string& url, const std::string& baseUrl) {
    if (url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://") {
        return url;  // Already absolute
    }
    
    if (url[0] == '/') {
        // Relative to domain root
        size_t pos = baseUrl.find("://");
        if (pos != std::string::npos) {
            pos = baseUrl.find('/', pos + 3);
            if (pos != std::string::npos) {
                return baseUrl.substr(0, pos) + url;
            }
        }
        return baseUrl + url;
    }
    
    // Relative to current directory
    return baseUrl + "/" + url;
}

void WebScraper::respectRateLimit() {
    static auto lastRequest = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRequest);
    
    if (elapsed.count() < delay_ms_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms_ - elapsed.count()));
    }
    
    lastRequest = std::chrono::steady_clock::now();
}

} // namespace etl
