#pragma once

#include <json/json.h>

#include <atomic>
#include <string>
#include <vector>

struct LlmProviderConfig {
    std::string name;
    std::string base_url;
    std::string api_key;
    std::string model;
    std::string path;
};

class LlmProviderPool {
public:
    bool InitFromConfig();

    bool Chat(const Json::Value& messages,
              std::string& reply,
              std::string& error,
              double temperature = 0.2);

    bool ExtractTodo(const std::string& text,
                     Json::Value& todo,
                     std::string& error);

private:
    bool CallProvider(const LlmProviderConfig& provider,
                      const Json::Value& messages,
                      std::string& reply,
                      std::string& error,
                      double temperature);

    static std::string BuildProviderUrl(const LlmProviderConfig& provider);
    static std::string Trim(const std::string& s);
    static bool ParseJsonLenient(const std::string& text, Json::Value& out);

private:
    std::vector<LlmProviderConfig> providers_;
    std::atomic<size_t> rr_index_{0};
    int timeout_ms_{15000};
    int max_retries_{2};
};
