#include "LlmProviderPool.h"

#include "ConfigMgr.h"

#include <curl/curl.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    const size_t total = size * nmemb;
    auto* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), total);
    return total;
}

std::vector<std::string> SplitByComma(const std::string& text) {
    std::vector<std::string> out;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) {
            out.push_back(item);
        }
    }
    return out;
}

std::string JsonToCompactString(const Json::Value& value) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

std::string ExtractErrorMessage(const Json::Value& root) {
    if (!root.isObject()) {
        return "Invalid upstream response";
    }
    const auto& err = root["error"];
    if (err.isObject() && err["message"].isString()) {
        return err["message"].asString();
    }
    if (err.isString()) {
        return err.asString();
    }
    return "Unknown upstream error";
}

std::string CurrentDateTimeString() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

}  // namespace

std::string LlmProviderPool::Trim(const std::string& s) {
    const auto begin = std::find_if_not(s.begin(), s.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();
    if (begin >= end) {
        return "";
    }
    return std::string(begin, end);
}

bool LlmProviderPool::InitFromConfig() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    auto& cfg = ConfigMgr::Inst();
    const auto timeout_text = cfg.GetValue("ProviderPool", "TimeoutMs");
    const auto retries_text = cfg.GetValue("ProviderPool", "MaxRetries");

    if (!timeout_text.empty()) {
        timeout_ms_ = std::max(2000, std::stoi(timeout_text));
    }
    if (!retries_text.empty()) {
        max_retries_ = std::max(1, std::stoi(retries_text));
    }

    std::vector<std::string> provider_sections;
    const auto provider_names = cfg.GetValue("ProviderPool", "Providers");
    if (!provider_names.empty()) {
        provider_sections = SplitByComma(provider_names);
        for (auto& name : provider_sections) {
            name = Trim(name);
        }
    }

    if (provider_sections.empty()) {
        for (int i = 1; i <= 32; ++i) {
            const std::string sec = "Provider" + std::to_string(i);
            if (!cfg.GetValue(sec, "BaseUrl").empty()) {
                provider_sections.push_back(sec);
            }
        }
    }

    for (const auto& sec_raw : provider_sections) {
        const std::string sec = Trim(sec_raw);
        if (sec.empty()) {
            continue;
        }

        LlmProviderConfig item;
        item.name = sec;
        item.base_url = Trim(cfg.GetValue(sec, "BaseUrl"));
        item.api_key = Trim(cfg.GetValue(sec, "ApiKey"));
        item.model = Trim(cfg.GetValue(sec, "Model"));
        item.path = Trim(cfg.GetValue(sec, "Path"));
        if (item.path.empty()) {
            item.path = "/v1/chat/completions";
        }

        if (item.base_url.empty() || item.api_key.empty() || item.model.empty()) {
            std::cerr << "[LLMServer] skip invalid provider section: " << sec << std::endl;
            continue;
        }

        providers_.push_back(std::move(item));
    }

    std::cout << "[LLMServer] loaded providers: " << providers_.size() << std::endl;
    return !providers_.empty();
}

std::string LlmProviderPool::BuildProviderUrl(const LlmProviderConfig& provider) {
    if (provider.base_url.empty()) {
        return "";
    }
    if (provider.base_url.back() == '/' && !provider.path.empty() && provider.path.front() == '/') {
        return provider.base_url.substr(0, provider.base_url.size() - 1) + provider.path;
    }
    if (provider.base_url.back() != '/' && !provider.path.empty() && provider.path.front() != '/') {
        return provider.base_url + "/" + provider.path;
    }
    return provider.base_url + provider.path;
}

bool LlmProviderPool::CallProvider(const LlmProviderConfig& provider,
                                   const Json::Value& messages,
                                   std::string& reply,
                                   std::string& error,
                                   double temperature) {
    Json::Value payload;
    payload["model"] = provider.model;
    payload["messages"] = messages;
    payload["temperature"] = temperature;

    const std::string request_body = JsonToCompactString(payload);
    const std::string url = BuildProviderUrl(provider);

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        error = "curl init failed";
        return false;
    }

    std::string response;
    struct curl_slist* headers = nullptr;
    const std::string auth = "Authorization: Bearer " + provider.api_key;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, auth.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(request_body.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms_);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, std::min(timeout_ms_, 5000));

    const CURLcode code = curl_easy_perform(curl);

    long http_status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (code != CURLE_OK) {
        error = std::string("curl error: ") + curl_easy_strerror(code);
        return false;
    }

    Json::Value root;
    if (!ParseJsonLenient(response, root)) {
        error = "invalid json from upstream";
        return false;
    }

    if (http_status < 200 || http_status >= 300) {
        error = ExtractErrorMessage(root);
        return false;
    }

    const auto& choices = root["choices"];
    if (!choices.isArray() || choices.empty()) {
        error = "choices is empty";
        return false;
    }

    const auto& first = choices[0];
    if (first["message"].isObject() && first["message"]["content"].isString()) {
        reply = first["message"]["content"].asString();
        return true;
    }
    if (first["text"].isString()) {
        reply = first["text"].asString();
        return true;
    }

    error = "no content in upstream response";
    return false;
}

bool LlmProviderPool::Chat(const Json::Value& messages,
                           std::string& reply,
                           std::string& error,
                           double temperature) {
    if (providers_.empty()) {
        error = "provider pool is empty";
        return false;
    }

    const size_t provider_count = providers_.size();
    const int attempts = std::max(max_retries_, static_cast<int>(provider_count));
    const size_t start = rr_index_.fetch_add(1);

    for (int i = 0; i < attempts; ++i) {
        const size_t idx = (start + static_cast<size_t>(i)) % provider_count;
        if (CallProvider(providers_[idx], messages, reply, error, temperature)) {
            return true;
        }
    }

    return false;
}

bool LlmProviderPool::ParseJsonLenient(const std::string& text, Json::Value& out) {
    Json::CharReaderBuilder builder;
    std::string errs;
    std::istringstream ss(text);
    if (Json::parseFromStream(builder, ss, &out, &errs)) {
        return true;
    }

    const auto start = text.find('{');
    const auto end = text.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        return false;
    }

    std::istringstream ss2(text.substr(start, end - start + 1));
    return Json::parseFromStream(builder, ss2, &out, &errs);
}

bool LlmProviderPool::ExtractTodo(const std::string& text,
                                  Json::Value& todo,
                                  std::string& error) {
    Json::Value messages(Json::arrayValue);
    const std::string now_text = CurrentDateTimeString();

    Json::Value system;
    system["role"] = "system";
    system["content"] =
        "你是办公助手。请从用户输入中提取待办信息，并且只输出JSON对象，不要输出其他文本。"
        "字段必须包含:title,event,location,time_text,start_time,end_time,confidence。"
        "时间格式统一为YYYY-MM-DD HH:MM:SS；无法确定时填空字符串。"
        "当前时间是 " + now_text + "，必须按当前时间解释“今天/明天/下周一”等相对时间。";
    messages.append(system);

    Json::Value user;
    user["role"] = "user";
    user["content"] = text;
    messages.append(user);

    std::string reply;
    if (!Chat(messages, reply, error, 0.1)) {
        return false;
    }

    Json::Value parsed;
    if (!ParseJsonLenient(reply, parsed) || !parsed.isObject()) {
        error = "todo json parse failed";
        return false;
    }

    const auto fallback_title = text.substr(0, std::min<size_t>(36, text.size()));

    todo["title"] = parsed["title"].isString() ? parsed["title"].asString() : fallback_title;
    todo["event"] = parsed["event"].isString() ? parsed["event"].asString() : text;
    todo["location"] = parsed["location"].isString() ? parsed["location"].asString() : "";
    todo["time_text"] = parsed["time_text"].isString() ? parsed["time_text"].asString() : "";
    todo["start_time"] = parsed["start_time"].isString() ? parsed["start_time"].asString() : "";
    todo["end_time"] = parsed["end_time"].isString() ? parsed["end_time"].asString() : "";
    todo["confidence"] = parsed["confidence"].isNumeric() ? parsed["confidence"].asDouble() : 0.4;

    return true;
}
